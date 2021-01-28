/*
 * fill.c - byte unpacking 
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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#define DEBUG
#include "common/conventions.h"
#include "metablock.h"
#include "archive.h"
#include "misc.h"

#include "fill.h"

void common_inode_fill(struct common_inode *c, unsigned char *src) {
c->type=getu16(src); src+=2;
c->permissions=getu16(src); src+=2;
c->uidindex=getu16(src); src+=2;
c->gidindex=getu16(src); src+=2;
c->mtime=getu32(src); src+=4;
c->inode_number=getu32(src);
}

void basicdirectory_inode_fill(struct basicdirectory_inode *b, unsigned char *src) {
b->block_index=getu32(src); src+=4;
b->link_count=getu32(src); src+=4;
b->file_size=getu16(src); src+=2;
b->block_offset=getu16(src); src+=2;
b->parent_inode=getu32(src);
}

void extendeddirectory_inode_fill(struct extendeddirectory_inode *e, unsigned char *src) {
e->link_count=getu32(src); src+=4;
e->file_size=getu32(src); src+=4;
e->block_index=getu32(src); src+=4;
e->parent_inode=getu32(src); src+=4;
e->index_count=getu16(src); src+=2;
e->block_offset=getu16(src); src+=2;
e->xattr_index=getu32(src);
}

void basicfile_inode_fill(struct basicfile_inode *b, unsigned char *src) {
b->blocks_start=getu32(src); src+=4;
b->frag_index=getu32(src); src+=4;
b->block_offset=getu32(src); src+=4;
b->file_size=getu32(src);
}

void extendedfile_inode_fill(struct extendedfile_inode *b, unsigned char *src) {
b->blocks_start=getu64(src); src+=8;
b->file_size=getu64(src); src+=8;
b->sparse=getu64(src); src+=8;
b->link_count=getu32(src); src+=4;
b->frag_index=getu32(src); src+=4;
b->block_offset=getu32(src); src+=4;
b->xattr_index=getu32(src);
}

void basicsymlink_inode_fill(struct basicsymlink_inode *b, unsigned char *src) {
b->link_count=getu32(src); src+=4;
b->target_size=getu32(src);
}

void extendedsymlink_inode_fill(struct extendedsymlink_inode *b, unsigned char *src) {
b->link_count=getu32(src); src+=4;
b->target_size=getu32(src); src+=4;
b->xattr_index=getu32(src);
}

void basicdevice_inode_fill(struct basicdevice_inode *b, unsigned char *src) {
b->link_count=getu32(src); src+=4;
b->device_number=getu32(src);
}

void extendeddevice_inode_fill(struct extendeddevice_inode *b, unsigned char *src) {
b->link_count=getu32(src); src+=4;
b->device_number=getu32(src); src+=4;
b->xattr_index=getu32(src);
}

void basicspecial_inode_fill(struct basicspecial_inode *b, unsigned char *src) {
b->link_count=getu32(src);
}

void extendedspecial_inode_fill(struct extendedspecial_inode *b, unsigned char *src) {
b->link_count=getu32(src); src+=4;
b->xattr_index=getu32(src);
}

void superblock_fill(struct superblock *s, unsigned char *buffer) {
s->magic=getu32(buffer+0);
s->inode_count=getu32(buffer+4);
s->modification_time=getu32(buffer+8);
s->block_size=getu32(buffer+12);
s->fragment_entry_count=getu32(buffer+16);
s->compression_id=getu16(buffer+20);
s->block_log=getu16(buffer+22);
s->flags=getu16(buffer+24);
s->id_count=getu16(buffer+26);
s->version_major=getu16(buffer+28);
s->version_minor=getu16(buffer+30);
s->root_inode_ref=getu64(buffer+32);
s->bytes_used=getu64(buffer+40);
s->id_table_start=getu64(buffer+48);
s->attr_id_table_start=getu64(buffer+56);
s->inode_table_start=getu64(buffer+64);
s->directory_table_start=getu64(buffer+72);
s->fragment_table_start=getu64(buffer+80);
s->export_table_start=getu64(buffer+88);
}

void header_dirent_fill(struct header_dirent *h, unsigned char *buffer) {
h->countm1=getu32(buffer);
h->start=getu32(buffer+4);
h->inode_number=getu32(buffer+8);
}

void entry_dirent_fill(struct entry_dirent *e, unsigned char *buffer) {
e->offset=getu16(buffer+0);
e->inode_offset=(int16_t)getu16(buffer+2);
e->type=getu16(buffer+4);
e->name_sizem1=getu16(buffer+6);
}

void header_fragment_fill(struct header_fragment *h, unsigned char *src) {
h->start=getu64(src); src+=8;
h->size=getu32(src); src+=4;
h->unused=getu32(src);
}
