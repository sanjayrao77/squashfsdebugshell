/*
 * director.c - manages directories in archive
 * Copyright (C) 2021 Sanjay Rao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#define DEBUG
#include "common/conventions.h"
#include "metablock.h"
#include "archive.h"
#include "cursor.h"
#include "print.h"
#include "fill.h"

#include "directory.h"

SICLEARFUNC(metablock_cursor);

static int setdirectory(struct archive *a, struct inode *inode) {
switch (inode->type) {
	case BASICDIRECTORY_TYPE : 
		a->directory.block_index=inode->basicdirectory.block_index;
		a->directory.block_offset=inode->basicdirectory.block_offset;
		a->directory.file_size=inode->basicdirectory.file_size;
		break;
	case EXTENDEDDIRECTORY_TYPE : 
		a->directory.block_index=inode->extendeddirectory.block_index;
		a->directory.block_offset=inode->extendeddirectory.block_offset;
		a->directory.file_size=inode->extendeddirectory.file_size;
		break;
	default:
		fprintf(stderr,"Attempt to read a directory on %s\n",typetostring_archive(inode->type,"Unknown"));
		GOTOERROR;
}
a->directory.bytesleft=a->directory.file_size;
a->directory.entry_count=0;
return 0;
error:
	return -1;
}

static int readheader(struct archive *a, struct metablock_cursor *c) {
unsigned char buffer[SIZE_HEADER_DIRENT];
if (readbytes_metablock_cursor(buffer,c,a,SIZE_HEADER_DIRENT)) GOTOERROR;
(void)header_dirent_fill(&a->directory.header,buffer);
a->directory.entry_count=a->directory.header.countm1+1;
a->directory.bytesleft-=SIZE_HEADER_DIRENT;
return 0;
error:
	return -1;
}

static int readdirent(struct archive *a, struct metablock_cursor *c) {
unsigned char buffer[SIZE_ENTRY_DIRENT];
unsigned int nsize;
if (readbytes_metablock_cursor(buffer,c,a,SIZE_ENTRY_DIRENT)) GOTOERROR;
(void)entry_dirent_fill(&a->directory.entry,buffer);
nsize=a->directory.entry.name_sizem1+1;
if (nsize>MAXSIZE_FILENAME) GOTOERROR;
a->filenamebuffer[nsize]='\0';
if (readbytes_metablock_cursor((unsigned char *)a->filenamebuffer,c,a,nsize)) GOTOERROR;
a->directory.bytesleft-=nsize+SIZE_ENTRY_DIRENT;
return 0;
error:
	return -1;
}

int step_list_directory(int *isdone_out, struct archive *a, int isverbose) {
struct metablock_cursor *c;
int isdone=0;

c=&a->directory.cursor;
if (!c->cache) {
	if (isverbose) printf("Directory is empty\n");
	return 0;
}

if (!a->directory.entry_count) {
	if (a->directory.bytesleft<SIZE_HEADER_DIRENT) {
		isdone=1;
		deinit_metablock_cursor(c); c->cache=NULL;
	} else {
		if (readheader(a,c)) GOTOERROR;
//		print_header_dirent(&a->directory.header);
		if (isverbose) oneline_print_header_dirent(&a->directory.header);
	}
} else {
	if (a->directory.bytesleft<=SIZE_ENTRY_DIRENT) GOTOERROR;
	if (readdirent(a,c)) GOTOERROR;
//	print_entry_dirent(&a->directory.entry,&a->directory.header,a->filenamebuffer);
	print_combined_dirent(a,&a->directory.header,&a->directory.entry,a->filenamebuffer);
	a->directory.entry_count-=1;
}

*isdone_out=isdone;
return 0;
error:
	return -1;
}
static int step_directory(int *isdone_out, struct archive *a, struct metablock_cursor *c) {
int isdone=0;

if (!a->directory.entry_count) {
	if (a->directory.bytesleft<SIZE_HEADER_DIRENT) isdone=1;
	else {
		if (readheader(a,c)) GOTOERROR;
//		print_header_dirent(&a->directory.header);
	}
} else {
	if (a->directory.bytesleft<=SIZE_ENTRY_DIRENT) GOTOERROR;
	if (readdirent(a,c)) GOTOERROR;
//	print_entry_dirent(&a->directory.entry,&a->directory.header,a->filenamebuffer);
	a->directory.entry_count-=1;
}

*isdone_out=isdone;
return 0;
error:
	return -1;
}

int list_directory(struct archive *a, struct inode *inode) {
struct metablock_cursor *c;

c=&a->directory.cursor;
if (c->cache) {
	deinit_metablock_cursor(c);
	clear_metablock_cursor(c);
}

if (setdirectory(a,inode)) {
	print_inode(a,inode);
	GOTOERROR;
}

if (init_metablock_cursor(c,a,a->superblock.directory_table_start+a->directory.block_index,a->directory.block_offset)) GOTOERROR;
#if 0
while (1) {
	int isdone;
//	fprintf(stderr,"bytesleft: %u\n",a->directory.bytesleft);
	if (step_list_directory(&isdone,a)) GOTOERROR;
	if (isdone) {
		if (a->directory.bytesleft) printf("! archive error, a->directory.bytesleft: %u\n",a->directory.bytesleft);
		break;
	}
}
deinit_metablock_cursor(c); c->cache=NULL;
#endif
return 0;
error:
	deinit_metablock_cursor(c); c->cache=NULL;
	return -1;
}

static int findentry(int *isfound_out, struct archive *a, char *match) {
// directory should be loaded into a.inode
struct metablock_cursor *c;
struct inode *inode;
int isfound=0;

c=&a->directory.cursor;
if (c->cache) {
	deinit_metablock_cursor(c);
	clear_metablock_cursor(c);
}

inode=&a->inode;
if (setdirectory(a,inode)) {
	print_inode(a,inode);
	GOTOERROR;
}

if (init_metablock_cursor(c,a,a->superblock.directory_table_start+a->directory.block_index,a->directory.block_offset)) GOTOERROR;
a->filenamebuffer[0]='\0';
while (1) {
	int isdone;
	if (step_directory(&isdone,a,c)) GOTOERROR;
	if (isdone) break;
	if (!strcmp(a->filenamebuffer,match)) {
		isfound=1;
		break;
	}
}
deinit_metablock_cursor(c); c->cache=NULL;
*isfound_out=isfound;
return 0;
error:
	deinit_metablock_cursor(c); c->cache=NULL;
	return -1;
}

static int enterdirectory(int *isfound_out, struct archive *a, char *dirname) {
// the pwd needs to be just-loaded in a->inode
int isfound;

if (!strcmp(dirname,".")) { *isfound_out=1; return same_directory(a); }
if (!strcmp(dirname,"..")) { *isfound_out=1; return back_directory(a); }

if (a->pwd.depth==MAXDEPTH_DIRLIST) GOTOERROR;

if (findentry(&isfound,a,dirname)) GOTOERROR;
if (isfound) {
	if (!isdirectory_archive(a->directory.entry.type)) {
		isfound=0;
		fprintf(stderr,"! error, %s is not a directory\n",dirname);
	} else {
		struct pointer_metablock *p;
		char *pwdname;
		int n,m;
		pwdname=a->pwd.pwdname;
		n=strlen(pwdname);
		m=strlen(dirname);
		if (n+m+1>MAX_PWDNAME) GOTOERROR;
		pwdname[n]='/';
		pwdname++;
		memcpy(pwdname+n,dirname,m+1);
		p=&a->pwd.dirlist[++a->pwd.depth].pointer;
		p->blocksoffset=a->directory.header.start;
		p->offsetinblock=a->directory.entry.offset;
		if (loadinode_archive(a,p)) GOTOERROR;
	}
}
*isfound_out=isfound;
return 0;
error:
	return -1;
}

int back_directory(struct archive *a) {
char *temp;
if (!a->pwd.depth) return 0;
a->pwd.depth--;
temp=strrchr(a->pwd.pwdname,'/');
if (!temp) GOTOERROR;
*temp='\0';
if (loadinode_archive(a,&a->pwd.dirlist[a->pwd.depth].pointer)) GOTOERROR;
return 0;
error:
	return -1;
}
int same_directory(struct archive *a) {
// this can be useful to reload the current directory after loading other inodes
if (loadinode_archive(a,&a->pwd.dirlist[a->pwd.depth].pointer)) GOTOERROR;
return 0;
error:
	return -1;
}

int chdir_directory(struct archive *a, char *dirname) {
char path[MAX_PWDNAME+1];
int n;

n=strlen(dirname);
if (n>MAX_PWDNAME) GOTOERROR;
memcpy(path,dirname,n+1);
dirname=path;
if (*dirname=='/') {
	(void)setrootdirectory_archive(a);
	dirname++;
}
if (same_directory(a)) GOTOERROR;
while (1) {
	char *temp;
	int isfound;
	if (!*dirname) break;
	temp=strchr(dirname,'/');
	if (temp) *temp='\0';
	if (enterdirectory(&isfound,a,dirname)) GOTOERROR;
	if (!isfound) break;
	if (!temp) break;
	dirname=temp+1;
}

return 0;
error:
	return -1;
}

int catfile_directory(struct archive *a, char *filename) {
int isfound;
if (same_directory(a)) GOTOERROR;
if (findentry(&isfound,a,filename)) GOTOERROR;
if (isfound) {
	struct pointer_metablock p;
	p.blocksoffset=a->directory.header.start;
	p.offsetinblock=a->directory.entry.offset;
	if (freadfile_archive(a,&p,stdout)) GOTOERROR;
	fputc('\n',stdout);
}
return 0;
error:
	return -1;
}
