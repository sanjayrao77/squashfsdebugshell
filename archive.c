/*
 * archive.c - archive structure
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>
#define DEBUG
#include "common/conventions.h"
#include "misc.h"
#include "metablock.h"

#include "archive.h"

#include "fill.h"
#include "print.h"
#include "cursor.h"

SICLEARFUNC(metablock_cursor);

char typetoletter_archive(int type) {
// this is useful for ls listing
switch (type) {
	case EXTENDEDDIRECTORY_TYPE :
	case BASICDIRECTORY_TYPE : return 'd';
	case EXTENDEDFILE_TYPE :
	case BASICFILE_TYPE : return '-';
	case EXTENDEDSYMLINK_TYPE : 
	case BASICSYMLINK_TYPE : return 'l';
	case EXTENDEDBDEVICE_TYPE : 
	case BASICBDEVICE_TYPE : return 'b';
	case EXTENDEDCDEVICE_TYPE : 
	case BASICCDEVICE_TYPE : return 'c';
	case EXTENDEDNAMEDPIPE_TYPE : 
	case BASICNAMEDPIPE_TYPE : return 'p';
	case EXTENDEDSOCKET_TYPE : 
	case BASICSOCKET_TYPE : return 's';
}
return '?';;
}
char *typetostring_archive(int type, char *unk) {
switch (type) {
	case BASICDIRECTORY_TYPE : return "basic directory";
	case BASICFILE_TYPE : return "basic file";
	case BASICSYMLINK_TYPE : return "basic symlink";
	case BASICBDEVICE_TYPE : return "basic block device";
	case BASICCDEVICE_TYPE : return "basic character device";
	case BASICNAMEDPIPE_TYPE : return "basic fifo";
	case BASICSOCKET_TYPE : return "basic socket";
	case EXTENDEDDIRECTORY_TYPE : return "extended directory";
	case EXTENDEDFILE_TYPE : return "extended file";
	case EXTENDEDSYMLINK_TYPE : return "extended symlink";
	case EXTENDEDBDEVICE_TYPE : return "extended block device";
	case EXTENDEDCDEVICE_TYPE : return "extended character device";
	case EXTENDEDNAMEDPIPE_TYPE : return "extended fifo";
	case EXTENDEDSOCKET_TYPE : return "extended socket";
}
return unk;
}


void reftopointer_archive(struct pointer_metablock *p_out, uint64_t inoderef) {
// 64bitref: 16: unused, 32: blocksoffset, 16: offsetinblock
p_out->blocksoffset=(uint32_t)((inoderef&0x0000ffffffff0000)>>16);
p_out->offsetinblock=(uint16_t)(inoderef&0xffff);
}

int ref_loadinode_archive(struct archive *a, uint64_t inoderef) {
struct pointer_metablock pointer;
(void)reftopointer_archive(&pointer,inoderef);
return loadinode_archive(a,&pointer);
}

static int readsuperblock(struct archive *a) {
unsigned char buffer[SIZE_SUPERBLOCK];
if (a->reader.readoff(a->reader.opt,buffer,0,SIZE_SUPERBLOCK)) GOTOERROR;
(void)superblock_fill(&a->superblock,buffer);
a->dataoffset=SIZE_SUPERBLOCK;
if (a->superblock.flags&COMPRESSOROPTIONS_FLAG_SUPERBLOCK) {
	switch (a->superblock.compression_id) {
		case GZIP_COMPRESSION_ID_SUPERBLOCK:
			a->dataoffset+=SIZE_GZIP_COMPRESSOROPTIONS;
	#if 0
			// TODO, switch from uncompress to zlib streams and deflateReset the stream each time so we can re-use structures
			if (readgzipoptions(a,a->fd)) GOTOERROR;
	#endif
			break;
		default:
			fprintf(stderr,"Unhandled compression_id: %u\n",a->superblock.compression_id);
			GOTOERROR;
	}
}
if (a->superblock.block_size>1024*1024) GOTOERROR; // sanity check, larger sizes are fine but non-spec
return 0;
error:
	return -1;
}


int init_archive(struct archive *a, int (*readoff)(void *, unsigned char *, uint64_t, unsigned int), void *opt) {
unsigned int oneblocksize;

a->reader.readoff=readoff;
a->reader.opt=opt;

if (readsuperblock(a)) GOTOERROR;
oneblocksize = a->superblock.block_size;
if (oneblocksize<8192) oneblocksize=8192;
if (!(a->compressed.anyblock=malloc(3*oneblocksize))) GOTOERROR;
a->compressed.anyblock2=a->compressed.anyblock+oneblocksize;
a->fragment.block=a->compressed.anyblock2+oneblocksize;
a->metablocks.quotaleft=10; // maximum .left number of cache blocks. needs 3 or so to do a directory list
return 0;
error:
	return -1;
}
void deinit_archive(struct archive *a) {
iffree(a->compressed.anyblock);
{
	struct cache_metablock *c,*n;
	c=a->metablocks.first;
	while (c) {
		n=c->listvars.next;
		free(c);
		c=n;
	}
}
}


static int readcommoninode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_COMMON_INODE)) return -1;
(void)common_inode_fill(&a->inode.common,a->structbuffer);
return 0;
}

static int readbasicdirectoryinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICDIRECTORY_INODE)) return -1;
a->inode.type=BASICDIRECTORY_TYPE;
(void)basicdirectory_inode_fill(&a->inode.basicdirectory,a->structbuffer);
return 0;
}

static int readextendeddirectoryinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDDIRECTORY_INODE)) return -1;
a->inode.type=EXTENDEDDIRECTORY_TYPE;
(void)extendeddirectory_inode_fill(&a->inode.extendeddirectory,a->structbuffer);
return 0;
}

static int readbasicfileinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICFILE_INODE)) return -1;
a->inode.type=BASICFILE_TYPE;
(void)basicfile_inode_fill(&a->inode.basicfile,a->structbuffer);
return 0;
}

static int step_readfile(uint64_t *fulloffset_inout, struct archive *a, struct metablock_cursor *c, unsigned int datasize) {
unsigned char buffer[4];
unsigned int sod;
int isuncompressed;
uint64_t fulloffset;

fulloffset=*fulloffset_inout;
if (readbytes_metablock_cursor(buffer,c,a,4)) GOTOERROR;
sod=getu32(buffer);
isuncompressed=sod&16777216;
sod=sod&16777215;
if (sod>datasize) GOTOERROR;
if (!sod) { // sparse, maybe not possible for basicfile
	memset(a->compressed.anyblock,0,datasize);
} else if (isuncompressed) { // uncompressed
	if (sod!=datasize) GOTOERROR;
	if (a->reader.readoff(a->reader.opt,a->compressed.anyblock,fulloffset,datasize)) GOTOERROR;
} else {
	uLongf destLen=datasize;
	if (sod==datasize) GOTOERROR; // not really an error but should have been stored uncompressed
	if (a->reader.readoff(a->reader.opt,a->compressed.anyblock2,fulloffset,sod)) GOTOERROR;
	if (Z_OK!=uncompress(a->compressed.anyblock,&destLen,a->compressed.anyblock2,sod)) GOTOERROR; 
	if (destLen!=datasize) GOTOERROR;
}
*fulloffset_inout=fulloffset+sod;
return 0;
error:
	return -1;
}

SICLEARFUNC(fragment_cursor);
static int freadbasicfileinode(struct archive *a, struct metablock_cursor *c, FILE *fout) {
struct basicfile_inode *b;
unsigned int bytesleft,blocksize;
uint64_t fulloffset;
struct fragment_cursor fc;

clear_fragment_cursor(&fc);

if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICFILE_INODE)) GOTOERROR;
a->inode.type=BASICFILE_TYPE;
(void)basicfile_inode_fill(&a->inode.basicfile,a->structbuffer);
b=&a->inode.basicfile;
bytesleft=b->file_size;
blocksize=a->superblock.block_size;
fulloffset=/* a->dataoffset + */ b->blocks_start;
while (bytesleft>blocksize) {
	if (step_readfile(&fulloffset,a,c,blocksize)) GOTOERROR;
	if (1!=fwrite(a->compressed.anyblock,blocksize,1,fout)) GOTOERROR;
	bytesleft-=blocksize;
}
if (bytesleft) { // tail
	if (b->frag_index==0xffffffff) {
		if (step_readfile(&fulloffset,a,c,bytesleft)) GOTOERROR;
		if (1!=fwrite(a->compressed.anyblock,bytesleft,1,fout)) GOTOERROR;
	} else {
		if (init_fragment_cursor(&fc,a,b->frag_index,b->block_offset)) GOTOERROR;
		while (bytesleft) {
			unsigned int k;
			k=fc.bytesleft;
			if (!k) {
				if (advance_fragment_cursor(&fc,a)) GOTOERROR;
				k=fc.bytesleft;
			}
			if (k>bytesleft) k=bytesleft;
			if (1!=fwrite(fc.cursor,k,1,fout)) GOTOERROR;
			bytesleft-=k;
		}
	}
}
deinit_fragment_cursor(&fc);
return 0;
error:
	deinit_fragment_cursor(&fc);
	return -1;
}
static int readextendedfileinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDFILE_INODE)) return -1;
a->inode.type=EXTENDEDFILE_TYPE;
(void)extendedfile_inode_fill(&a->inode.extendedfile,a->structbuffer);
return 0;
}
static int freadextendedfileinode(struct archive *a, struct metablock_cursor *c, FILE *fout) {
struct extendedfile_inode *e;
unsigned int bytesleft,blocksize;
uint64_t fulloffset;
struct fragment_cursor fc;

clear_fragment_cursor(&fc);

if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDFILE_INODE)) GOTOERROR;
a->inode.type=EXTENDEDFILE_TYPE;
(void)extendedfile_inode_fill(&a->inode.extendedfile,a->structbuffer);
e=&a->inode.extendedfile;
bytesleft=e->file_size;
blocksize=a->superblock.block_size;
fulloffset=/* a->dataoffset + */ e->blocks_start;
while (bytesleft>blocksize) {
	if (step_readfile(&fulloffset,a,c,blocksize)) GOTOERROR;
	if (1!=fwrite(a->compressed.anyblock,blocksize,1,fout)) GOTOERROR;
	bytesleft-=blocksize;
}
if (bytesleft) { // tail
	if (e->frag_index==0xffffffff) {
		if (step_readfile(&fulloffset,a,c,bytesleft)) GOTOERROR;
		if (1!=fwrite(a->compressed.anyblock,bytesleft,1,fout)) GOTOERROR;
	} else {
		if (init_fragment_cursor(&fc,a,e->frag_index,e->block_offset)) GOTOERROR;
		while (bytesleft) {
			unsigned int k;
			k=fc.bytesleft;
			if (!k) {
				if (advance_fragment_cursor(&fc,a)) GOTOERROR;
				k=fc.bytesleft;
			}
			if (k>bytesleft) k=bytesleft;
			if (1!=fwrite(fc.cursor,k,1,fout)) GOTOERROR;
			bytesleft-=k;
		}
	}
}
deinit_fragment_cursor(&fc);
return 0;
error:
	deinit_fragment_cursor(&fc);
	return -1;
}

static int readbasicsymlinkinode(struct archive *a, struct metablock_cursor *c) {
unsigned int size;
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICSYMLINK_INODE)) return -1;
a->inode.type=BASICSYMLINK_TYPE;
(void)basicsymlink_inode_fill(&a->inode.basicsymlink,a->structbuffer);

size=a->inode.basicsymlink.target_size;
if (size>MAXSIZE_FILENAME) return -1;
if (readbytes_metablock_cursor((unsigned char *)a->symlinkbuffer,c,a,size)) return -1;
a->symlinkbuffer[size]='\0';
return 0;
}
static int readextendedsymlinkinode(struct archive *a, struct metablock_cursor *c) {
struct extendedsymlink_inode e;
// TODO check to see if this is really how it's stored, according to format.txt
if (readbytes_metablock_cursor(a->structbuffer,c,a,8)) return -1;
(void)extendedsymlink_inode_fill(&e,a->structbuffer);
if (e.target_size>MAXSIZE_FILENAME) return -1;
if (readbytes_metablock_cursor((unsigned char *)a->symlinkbuffer,c,a,e.target_size)) return -1;
a->symlinkbuffer[e.target_size]='\0';
if (readbytes_metablock_cursor(a->structbuffer+8,c,a,4)) return -1;
a->inode.type=EXTENDEDSYMLINK_TYPE;
(void)extendedsymlink_inode_fill(&a->inode.extendedsymlink,a->structbuffer);
return 0;
}
static int readbasicbdeviceinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICDEVICE_INODE)) return -1;
a->inode.type=BASICBDEVICE_TYPE;
(void)basicdevice_inode_fill(&a->inode.basicbdevice,a->structbuffer);
return 0;
}
static int readextendedbdeviceinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDDEVICE_INODE)) return -1;
a->inode.type=EXTENDEDBDEVICE_TYPE;
(void)extendeddevice_inode_fill(&a->inode.extendedbdevice,a->structbuffer);
return 0;
}
static int readbasiccdeviceinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICDEVICE_INODE)) return -1;
a->inode.type=BASICCDEVICE_TYPE;
(void)basicdevice_inode_fill(&a->inode.basiccdevice,a->structbuffer);
return 0;
}
static int readextendedcdeviceinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDDEVICE_INODE)) return -1;
a->inode.type=EXTENDEDCDEVICE_TYPE;
(void)extendeddevice_inode_fill(&a->inode.extendedcdevice,a->structbuffer);
return 0;
}
static int readbasicnamedpipeinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICSPECIAL_INODE)) return -1;
a->inode.type=BASICNAMEDPIPE_TYPE;
(void)basicspecial_inode_fill(&a->inode.basicnamedpipe,a->structbuffer);
return 0;
}
static int readextendednamedpipeinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDSPECIAL_INODE)) return -1;
a->inode.type=EXTENDEDNAMEDPIPE_TYPE;
(void)extendedspecial_inode_fill(&a->inode.extendednamedpipe,a->structbuffer);
return 0;
}
static int readbasicsocketinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_BASICSPECIAL_INODE)) return -1;
a->inode.type=BASICSOCKET_TYPE;
(void)basicspecial_inode_fill(&a->inode.basicsocket,a->structbuffer);
return 0;
}
static int readextendedsocketinode(struct archive *a, struct metablock_cursor *c) {
if (readbytes_metablock_cursor(a->structbuffer,c,a,SIZE_EXTENDEDSPECIAL_INODE)) return -1;
a->inode.type=EXTENDEDSOCKET_TYPE;
(void)extendedspecial_inode_fill(&a->inode.extendedsocket,a->structbuffer);
return 0;
}

int loadinode_archive(struct archive *a, struct pointer_metablock *p) {
struct metablock_cursor cursor;

clear_metablock_cursor(&cursor);
if (init_metablock_cursor(&cursor,a,a->superblock.inode_table_start+p->blocksoffset,p->offsetinblock)) GOTOERROR;
if (readcommoninode(a,&cursor)) GOTOERROR;
a->inode.pointer=*p;
a->inode.type=a->inode.common.type;
switch (a->inode.type) {
	case BASICDIRECTORY_TYPE :
		if (readbasicdirectoryinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDDIRECTORY_TYPE :
		if (readextendeddirectoryinode(a,&cursor)) GOTOERROR;
		break;
	case BASICFILE_TYPE :
		if (readbasicfileinode(a,&cursor)) GOTOERROR;
		break;
	case BASICSYMLINK_TYPE : 
		if (readbasicsymlinkinode(a,&cursor)) GOTOERROR;
		break;
	case BASICBDEVICE_TYPE : 
		if (readbasicbdeviceinode(a,&cursor)) GOTOERROR;
		break;
	case BASICCDEVICE_TYPE : 
		if (readbasiccdeviceinode(a,&cursor)) GOTOERROR;
		break;
	case BASICNAMEDPIPE_TYPE : 
		if (readbasicnamedpipeinode(a,&cursor)) GOTOERROR;
		break;
	case BASICSOCKET_TYPE : 
		if (readbasicsocketinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDFILE_TYPE : 
		if (readextendedfileinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDSYMLINK_TYPE : 
		if (readextendedsymlinkinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDBDEVICE_TYPE : 
		if (readextendedbdeviceinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDCDEVICE_TYPE : 
		if (readextendedcdeviceinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDNAMEDPIPE_TYPE : 
		if (readextendednamedpipeinode(a,&cursor)) GOTOERROR;
		break;
	case EXTENDEDSOCKET_TYPE : 
		if (readextendedsocketinode(a,&cursor)) GOTOERROR;
		break;
	default:
		fprintf(stderr,"Unknown inode type %d\n",a->inode.common.type);
		GOTOERROR;
}
deinit_metablock_cursor(&cursor);
return 0;
error:
	deinit_metablock_cursor(&cursor);
	return -1;
}

int fetch_id(unsigned int *id_out, struct archive *a, unsigned short idindex) {
struct metablock_cursor c;
unsigned char buffer[8];
uint64_t blocksoffset;

clear_metablock_cursor(&c);

if (idindex>=a->superblock.id_count) {
	fprintf(stderr,"Invalid id request: %u, superblock.id_count is %u\n",idindex,a->superblock.id_count);
	GOTOERROR;
}
if (a->reader.readoff(a->reader.opt,buffer,a->superblock.id_table_start+8*(idindex/1024),8)) GOTOERROR;
blocksoffset=getu64(buffer);

if (init_metablock_cursor(&c,a,blocksoffset,0)) GOTOERROR;
memcpy(buffer,c.cursor+4*(idindex%1024),4);
*id_out=getu32(buffer);

deinit_metablock_cursor(&c);
return 0;
error:
	deinit_metablock_cursor(&c);
	return -1;
}

void setrootdirectory_archive(struct archive *a) {
(void)reftopointer_archive(&a->pwd.dirlist[0].pointer,a->superblock.root_inode_ref);
a->pwd.pwdname[0]='\0';
a->pwd.depth=0;
}

int isdirectory_archive(unsigned int type) {
if (type==BASICDIRECTORY_TYPE) return 1;
if (type==EXTENDEDDIRECTORY_TYPE) return 1;
return 0;
}


unsigned int major_archive(unsigned int devno) {
return (devno&0xfff00)>>8;
}
unsigned int minor_archive(unsigned int devno) {
return ((devno&0xfff00000)>>12)|(devno&0xff);
}

int freadfile_archive(struct archive *a, struct pointer_metablock *p, FILE *fout) {
struct metablock_cursor cursor;

clear_metablock_cursor(&cursor);
if (init_metablock_cursor(&cursor,a,a->superblock.inode_table_start+p->blocksoffset,p->offsetinblock)) GOTOERROR;
if (readcommoninode(a,&cursor)) GOTOERROR;
a->inode.pointer=*p;
a->inode.type=a->inode.common.type;
switch (a->inode.type) {
	case BASICFILE_TYPE :
		if (freadbasicfileinode(a,&cursor,fout)) GOTOERROR;
		break;
	case EXTENDEDFILE_TYPE : 
		if (freadextendedfileinode(a,&cursor,fout)) GOTOERROR;
		break;
	default:
		fprintf(stderr,"Not a file %s\n",typetostring_archive(a->inode.common.type,"Unknown"));
		GOTOERROR;
}
deinit_metablock_cursor(&cursor);
return 0;
error:
	deinit_metablock_cursor(&cursor);
	return -1;
}
