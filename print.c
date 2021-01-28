/*
 * print.c - human-readable displays of structs
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
#include <endian.h>
#include <time.h>
#define DEBUG
#include "common/conventions.h"
#include "metablock.h"
#include "archive.h"

#include "print.h"

int print_superblock(struct superblock *s) {
printf("struct superblock {\n"\
"	uint32_t magic\t\t\t= %u;\n"\
"	uint32_t inode_count\t\t= %u;\n"\
"	uint32_t modification_time\t= %u;\n"\
"	uint32_t block_size\t\t= %u;\n"\
"	uint32_t fragment_entry_count\t= %u;\n"\
"	uint16_t compression_id\t\t= %u;\n"\
"	uint16_t block_log\t\t= %u;\n"\
"	uint16_t flags\t\t\t= 0x%x;\n"\
"	uint16_t id_count\t\t= %u;\n"\
"	uint16_t version_major\t= %u, version_minor\t= %u;\n"\
"	uint64_t root_inode_ref\t\t= %"PRIu64";\n"\
"	uint64_t bytes_used\t\t= %"PRIu64";\n"\
"	uint64_t id_table_start\t\t= 0x%"PRIx64";\n"\
"	uint64_t attr_id_table_start\t= 0x%"PRIx64";\n"\
"	uint64_t inode_table_start\t= 0x%"PRIx64";\n"\
"	uint64_t directory_table_start\t= 0x%"PRIx64";\n"\
"	uint64_t fragment_table_start\t= 0x%"PRIx64";\n"\
"	uint64_t export_table_start\t= 0x%"PRIx64";\n"\
"};\n",
s->magic,
s->inode_count,
s->modification_time,
s->block_size,
s->fragment_entry_count,
s->compression_id,
s->block_log,
s->flags,
s->id_count,
s->version_major,
s->version_minor,
s->root_inode_ref,
s->bytes_used,
s->id_table_start,
s->attr_id_table_start,
s->inode_table_start,
s->directory_table_start,
s->fragment_table_start,
s->export_table_start);

return 0;
}

static void printindent(unsigned int indent) {
while (indent) { indent--; fputc('\t',stdout); }
}

static int print_basicdirectory_inode(struct archive *a, struct basicdirectory_inode *b, unsigned int indent) {
printindent(indent); printf("struct basicdirectory_inode {\n");
indent+=1;
printindent(indent); printf("uint32_t block_index = %u\n",b->block_index);
printindent(indent); printf("uint32_t link_count = %u\n",b->link_count);
printindent(indent); printf("uint16_t file_size = %u\n",b->file_size);
printindent(indent); printf("uint16_t block_offset = %u\n",b->block_offset);
if (!b->parent_inode || (b->parent_inode > a->superblock.inode_count)) {
	printindent(indent); printf("uint32_t parent_inode = %u (this is root)\n",b->parent_inode);
} else {
	printindent(indent); printf("uint32_t parent_inode = %u\n",b->parent_inode);
}
indent-=1;
printindent(indent); printf("};\n");
return 0;
}

static int print_extendeddirectory_inode(struct archive *a, struct extendeddirectory_inode *b, unsigned int indent) {
printindent(indent); printf("struct extendeddirectory_inode {\n");
indent+=1;
printindent(indent); printf("uint32_t link_count = %u\n",b->link_count);
printindent(indent); printf("uint32_t file_size = %u\n",b->file_size);
printindent(indent); printf("uint32_t block_index = %u\n",b->block_index);
if (!b->parent_inode || (b->parent_inode > a->superblock.inode_count)) {
	printindent(indent); printf("uint32_t parent_inode = %u (this is root)\n",b->parent_inode);
} else {
	printindent(indent); printf("uint32_t parent_inode = %u\n",b->parent_inode);
}
printindent(indent); printf("uint16_t index_count = %u\n",b->index_count);
printindent(indent); printf("uint16_t block_offset = %u\n",b->block_offset);
printindent(indent); printf("uint32_t xattr_index = %u\n",b->xattr_index);
indent-=1;
printindent(indent); printf("};\n");
return 0;
}

int print_common_inode(struct archive *a, struct common_inode *c, unsigned int indent) {
unsigned int id;
printindent(indent); printf("struct common_inode {\n");
indent+=1;
printindent(indent); printf("uint16_t type = %u (%s)\n",c->type,typetostring_archive(c->type,"Unknown"));
printindent(indent); printf("uint16_t permissions = %o\n",c->permissions);
if (fetch_id(&id,a,c->uidindex)) GOTOERROR;
printindent(indent); printf("uint16_t uidindex = %u -> %u\n",c->uidindex,id);
if (fetch_id(&id,a,c->gidindex)) GOTOERROR;
printindent(indent); printf("uint16_t gidindex = %u -> %u\n",c->gidindex,id);
printindent(indent); printf("uint32_t mtime = %u\n",c->mtime);
printindent(indent); printf("uint32_t inode_number = %u\n",c->inode_number);
indent-=1;
printindent(indent); printf("};\n");
return 0;
error:
	return -1;
}

int print_inode(struct archive *a, struct inode *i) {
fprintf(stdout,"struct inode {\n");
print_common_inode(a,&i->common,1);
switch (i->type) {
	case BASICDIRECTORY_TYPE : print_basicdirectory_inode(a,&i->basicdirectory,1); break;
#if 0
	case BASICFILE_TYPE : print_basicfile_inode(&i->basicfile,1); break;
	case BASICSYMLINK_TYPE : print_basicsymlink_inode(&i->basicsymlink,1); break;
	case BASICBDEVICE_TYPE : print_basicbdevice_inode(&i->basicbdevice,1); break;
	case BASICCDEVICE_TYPE : print_basiccdevice_inode(&i->basiccdevice,1); break;
	case BASICNAMEDPIPE_TYPE : print_basicnamedpipe_inode(&i->basicnamedpipe,1); break;
	case BASICSOCKET_TYPE : print_basicsocket_inode(&i->basicsocket,1); break;
#endif
	case EXTENDEDDIRECTORY_TYPE : print_extendeddirectory_inode(a,&i->extendeddirectory,1); break;
#if 0
	case EXTENDEDFILE_TYPE : print_extendedfile_inode(&i->extendedfile,1); break;
	case EXTENDEDSYMLINK_TYPE : print_extendedsymlink_inode(&i->extendedsymlink,1); break;
	case EXTENDEDBDEVICE_TYPE : print_extendedbdevice_inode(&i->extendedbdevice,1); break;
	case EXTENDEDCDEVICE_TYPE : print_extendedcdevice_inode(&i->extendedcdevice,1); break;
	case EXTENDEDNAMEDPIPE_TYpe : print_extendednamedpipe_inode(&i->extendednamedpipe,1); break;
	case EXTENDEDSOCKET_TYPE : print_extendedsocket_inode(&i->extendedsocket,1); break;
#endif
}
fprintf(stdout,"} %u.%u;\n",i->pointer.blocksoffset,i->pointer.offsetinblock);
return 0;
}


int print_entry_dirent(struct entry_dirent *e, struct header_dirent *h, char *filename) {
printf("struct entry_dirent {\n"\
	"\tuint16_t offset\t= %u\n"\
	"\tint16_t inode_offset\t= %d -> %u\n"\
	"\tuint16_t type\t= %u (%s)\n"\
	"\tuint16_t name_sizem1\t= %u\n"\
	"\tu8[] name\t= \"%s\"\n"\
	"};\n",
	e->offset,e->inode_offset,
	h->inode_number+e->inode_offset,
	e->type,typetostring_archive(e->type,"Unknown"),e->name_sizem1,filename);
return 0;
}

int print_header_dirent(struct header_dirent *h) {
printf("struct header_dirent {\n"\
	"\tuint32_t countm1	= %u\n"\
	"\tuint32_t start	= %u\n"\
	"\tuint32_t inode_number	= %u\n"\
	"};\n", h->countm1, h->start, h->inode_number);
return 0;
}

int oneline_print_header_dirent(struct header_dirent *h) {
printf("struct header_dirent { "\
	"countm1	= %u, "\
	"start	= %u, "\
	"inode_number	= %u, "\
	"};\n", h->countm1, h->start, h->inode_number);
return 0;
}

int print_header_fragment(struct header_fragment *h) {
printf("struct header_fragment {\n"\
"\tuint64_t start\t= %"PRIu64"\n"\
"\tuint32_t size\t= %u, iscompressed: %c, real: %u\n"\
"\tuint32_t unused\t= %u\n"\
"};\n",h->start,h->size,(h->size&(1<<24))?'N':'Y',h->size&16777215,h->unused);
return 0;
}

int print_permissions(unsigned int m) {
fputc( (m&S_IRUSR)?'r':'-' ,stdout);
fputc( (m&S_IWUSR)?'w':'-' ,stdout);
fputc( (m&S_IXUSR)?'x':'-' ,stdout);
fputc( (m&S_IRGRP)?'r':'-' ,stdout);
fputc( (m&S_IWGRP)?'w':'-' ,stdout);
fputc( (m&S_IXGRP)?'x':'-' ,stdout);
fputc( (m&S_IROTH)?'r':'-' ,stdout);
fputc( (m&S_IWOTH)?'w':'-' ,stdout);
fputc( (m&S_IXOTH)?'x':'-' ,stdout);
return 0;
}

static void getlinkcount(unsigned int *linkcount_out, struct inode *i) {
switch (i->type) {
	case BASICDIRECTORY_TYPE: *linkcount_out=i->basicdirectory.link_count; break;
	case EXTENDEDDIRECTORY_TYPE: *linkcount_out=i->extendeddirectory.link_count; break;
	case EXTENDEDFILE_TYPE: *linkcount_out=i->extendedfile.link_count; break;
	case BASICSYMLINK_TYPE: *linkcount_out=i->basicsymlink.link_count; break;
	case EXTENDEDSYMLINK_TYPE: *linkcount_out=i->extendedsymlink.link_count; break;
	case BASICBDEVICE_TYPE: *linkcount_out=i->basicbdevice.link_count; break;
	case EXTENDEDBDEVICE_TYPE: *linkcount_out=i->extendedbdevice.link_count; break;
	case BASICCDEVICE_TYPE: *linkcount_out=i->basiccdevice.link_count; break;
	case EXTENDEDCDEVICE_TYPE: *linkcount_out=i->extendedcdevice.link_count; break;
	case BASICNAMEDPIPE_TYPE: *linkcount_out=i->basicnamedpipe.link_count; break;
	case EXTENDEDNAMEDPIPE_TYPE: *linkcount_out=i->extendednamedpipe.link_count; break;
	case BASICSOCKET_TYPE: *linkcount_out=i->basicsocket.link_count; break;
	case EXTENDEDSOCKET_TYPE: *linkcount_out=i->extendedsocket.link_count; break;
	default: *linkcount_out=1;
}
}

static int printmiddle(struct inode *i) {
switch (i->type) {
	case BASICDIRECTORY_TYPE : printf("%16u",i->basicdirectory.file_size); break;
	case BASICFILE_TYPE : printf("%16u",i->basicfile.file_size); break;
	case BASICSYMLINK_TYPE : printf("%16u",i->basicsymlink.target_size); break;
	case BASICBDEVICE_TYPE : 
		printf("%6u, %8u",major_archive(i->basicbdevice.device_number),minor_archive(i->basicbdevice.device_number)); break;
	case BASICCDEVICE_TYPE : 
		printf("%6u, %8u",major_archive(i->basiccdevice.device_number),minor_archive(i->basiccdevice.device_number)); break;
	case BASICNAMEDPIPE_TYPE : printf("%16u",0); break;
	case BASICSOCKET_TYPE : printf("%16u",0); break;
	case EXTENDEDDIRECTORY_TYPE : printf("%16u",i->extendeddirectory.file_size); break;
	case EXTENDEDFILE_TYPE : printf("%16"PRIu64,i->extendedfile.file_size); break;
	case EXTENDEDSYMLINK_TYPE : printf("%16u",i->extendedsymlink.target_size); break;
	case EXTENDEDBDEVICE_TYPE : 
		printf("%6u, %8u",major_archive(i->extendedbdevice.device_number),minor_archive(i->basicbdevice.device_number)); break;
	case EXTENDEDCDEVICE_TYPE : 
		printf("%6u, %8u",major_archive(i->extendedcdevice.device_number),minor_archive(i->basiccdevice.device_number)); break;
	case EXTENDEDNAMEDPIPE_TYPE : printf("%16u",0); break;
	case EXTENDEDSOCKET_TYPE : printf("%16u",0); break;
}
return 0;
}

static int printend(struct archive *a, struct inode *i) {
switch (i->type) {
	case BASICSYMLINK_TYPE :
	case EXTENDEDSYMLINK_TYPE :
		printf(" -> %s\n",a->symlinkbuffer);
		break;
	default:
		fputc('\n',stdout);
		break;
}
return 0;
}

static void gettimestamp(char *str, time_t t) {
// str should have 18 bytes
char stamp[26];
ctime_r(&t,stamp);

memcpy(str,stamp+4,12);
memcpy(str+12,stamp+19,5);
str[17]='\0';
}
int print_combined_dirent(struct archive *a, struct header_dirent *h, struct entry_dirent *e, char *filename) {
struct pointer_metablock p;
struct inode *i;
unsigned int linkcount;
unsigned int uid,gid;
char timestamp[18];

p.blocksoffset=h->start;
p.offsetinblock=e->offset;
if (loadinode_archive(a,&p)) GOTOERROR;
i=&a->inode;

(void)getlinkcount(&linkcount,i);
if (fetch_id(&uid,a,i->common.uidindex)) GOTOERROR;
if (fetch_id(&gid,a,i->common.gidindex)) GOTOERROR;
(void)gettimestamp(timestamp,i->common.mtime);

fputc(typetoletter_archive(i->type),stdout);
print_permissions(i->common.permissions);
printf(" %4u %4u %4u ",linkcount,uid,gid);
printmiddle(i);
printf(" %s %s",timestamp,filename);
printend(a,i);
return 0;
error:
	return -1;
}

int print_pointer_metablock(struct pointer_metablock *ptr) {
printf("struct pointer_metablock {\n"\
"\tunsigned int blocksoffset\t= %u\n"\
"\tunsigned short offsetinblock\t= %u\n"\
"} %p;\n",
	ptr->blocksoffset,ptr->offsetinblock,ptr);
return 0;
}
