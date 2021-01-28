/*
 * cursor.c - manages walking through archive
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
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <zlib.h>
#define DEBUG
#include "common/conventions.h"
#include "metablock.h"
#include "archive.h"
#include "fill.h"
#include "misc.h"

#include "cursor.h"

SICLEARFUNC(cache_metablock);
SICLEARFUNC(metablock_cursor);

int init_metablock_cursor(struct metablock_cursor *c, struct archive *a, uint64_t fulloffset, unsigned int offsetinblock) {
unsigned char buff2[2];
unsigned short sod,isuncompressed;
struct cache_metablock *cache=NULL;
int leftused=0;

if (offsetinblock>=8192) GOTOERROR;
c->bytesleft=8192-offsetinblock;
cache=find_cache_metablock(a->metablocks.treetop,fulloffset);
if (cache) {
	cache->usecounter+=1;
	c->cache=cache;
	c->cursor=cache->uncompressed+offsetinblock;
	return 0;
}
if (a->metablocks.quotaleft) {
	if (!(cache=malloc(sizeof(struct cache_metablock)+8192))) GOTOERROR;
	clear_cache_metablock(cache);
	cache->uncompressed=(unsigned char *)(cache+1);
	leftused=1;
} else {
	cache=a->metablocks.first;
	while (1) {
		if (!cache->usecounter) break;
		cache=cache->listvars.next;
		if (!cache) {
			fprintf(stderr,"Metablock cache limit exceeded. Either there's a leak or the limit is too small for the operation.\n");
			GOTOERROR;
		}
	}
	*cache->listvars.ppnext=cache->listvars.next;
	if (cache->listvars.next) {
		cache->listvars.next->listvars.ppnext=cache->listvars.ppnext;
	}
	rmnode_cache_metablock(&a->metablocks.treetop,cache);
	cache->treevars.balance=0;
	cache->treevars.left=cache->treevars.right=NULL;
}

if (a->reader.readoff(a->reader.opt,buff2,fulloffset,2)) GOTOERROR;
sod=getu16(buff2);
// fprintf(stderr,"%s:%d reading metadata block size %u -> %u (0x%02x%02x)\n",__FILE__,__LINE__,sod,sod&32767,buff2[0],buff2[1]);
isuncompressed=sod&32768;
sod=sod&32767; // size on disk
if (isuncompressed) {
	if (a->reader.readoff(a->reader.opt,cache->uncompressed,fulloffset+2,sod)) GOTOERROR;
} else {
	uLongf destLen=8192;
	if (a->reader.readoff(a->reader.opt,a->compressed.anyblock,fulloffset+2,sod)) {
		fprintf(stderr,"%s:%d Trying to read at full offset: 0x%"PRIx64", sod:%u\n",__FILE__,__LINE__,fulloffset,sod);
		GOTOERROR;
	}
	if (Z_OK!=uncompress(cache->uncompressed,&destLen,a->compressed.anyblock,sod)) GOTOERROR; 
	if (destLen>8192) GOTOERROR;
	// there's no way to verify the final metablock size but every one up to that should be 8192
	// we _could_ tell if we're not in a final block by checking the superblock table sizes
}

a->metablocks.quotaleft-=leftused;

c->cursor=cache->uncompressed+offsetinblock;
c->cache=cache;

cache->thisblocksoffset=fulloffset;
cache->nextblocksoffset=fulloffset+2+sod;
cache->usecounter+=1;

(void)addnode_cache_metablock(&a->metablocks.treetop,cache);

if (!a->metablocks.last) {
	a->metablocks.first=a->metablocks.last=cache;
	cache->listvars.ppnext=&a->metablocks.first;
} else {
	a->metablocks.last->listvars.next=cache;
	cache->listvars.ppnext=&a->metablocks.last->listvars.next;
	cache->listvars.next=NULL;
	a->metablocks.last=cache;
}
return 0;
error:
	if (cache) {
		a->metablocks.quotaleft+=1;
		free(cache);
	}
	return -1;
}

void deinit_metablock_cursor(struct metablock_cursor *c) {
if (c->cache) {
	c->cache->usecounter-=1;
}
}

int advance_metablock_cursor(struct metablock_cursor *c, struct archive *a) {
uint64_t fulloffset;
fulloffset=c->cache->nextblocksoffset;
deinit_metablock_cursor(c);
clear_metablock_cursor(c);
if (init_metablock_cursor(c,a,fulloffset,0)) GOTOERROR;
return 0;
error:
	return -1;
}

int readbytes_metablock_cursor(unsigned char *dest, struct metablock_cursor *c, struct archive *a, unsigned int len) {
while (1) {
	unsigned int k;
	if (!len) break;
	k=c->bytesleft;
	if (!k) {
		if (advance_metablock_cursor(c,a)) GOTOERROR;
		k=c->bytesleft;
	}
	if (k>len) k=len;
	memcpy(dest,c->cursor,k);
	c->cursor+=k;
	c->bytesleft-=k;
	dest+=k;
	len-=k;
}
return 0;
error:
	return -1;
}

static int loadfragment(struct fragment_cursor *f, struct archive *a, unsigned int offsetinfragment) {
struct header_fragment header;
struct metablock_cursor c;

clear_metablock_cursor(&c);

if (init_metablock_cursor(&c,a,f->metablockoffset,0)) GOTOERROR;
(void)header_fragment_fill(&header,c.cursor+(f->fragindex%512)*SIZE_HEADER_FRAGMENT);
a->fragment.header=header;
if (header.size&(1<<24)) { // uncompressed
	unsigned int fragmentsize;
	fragmentsize=header.size&(16777216-1);
	if (fragmentsize>a->superblock.block_size) GOTOERROR;
	if (a->reader.readoff(a->reader.opt,a->fragment.block,header.start,fragmentsize)) GOTOERROR;
	a->fragment.blocksize=fragmentsize;
} else {
	uLongf destLen=a->superblock.block_size;
	if (a->reader.readoff(a->reader.opt,a->compressed.anyblock,header.start,header.size)) GOTOERROR;
	if (Z_OK!=uncompress(a->fragment.block,&destLen,a->compressed.anyblock,header.size)) GOTOERROR;
	a->fragment.blocksize=destLen;
}

f->cursor=a->fragment.block+offsetinfragment;
f->offset=offsetinfragment;
if (offsetinfragment>=a->fragment.blocksize) GOTOERROR; // == case is fine but it shouldn't happen
f->bytesleft=a->fragment.blocksize-offsetinfragment;

deinit_metablock_cursor(&c);
return 0;
error:
	deinit_metablock_cursor(&c);
	return -1;
}

int init_fragment_cursor(struct fragment_cursor *f, struct archive *a, unsigned int fragindex, unsigned int offsetinfragment) {
unsigned char buffer8[8];

if (fragindex>=a->superblock.fragment_entry_count) GOTOERROR;
f->fragindex=fragindex;
f->metablockindex=fragindex/512;
if (a->reader.readoff(a->reader.opt,buffer8,a->superblock.fragment_table_start+8*f->metablockindex,8)) GOTOERROR;
f->metablockoffset=getu64(buffer8);
if (loadfragment(f,a,offsetinfragment)) GOTOERROR;
return 0;
error:
	return -1;
}

#if 0
void deinit_fragment_cursor(struct fragment_cursor *f) {
}
#endif

#if 0
// TODO
int readbytes_fragment_cursor(unsigned char *dest, struct fragment_cursor *f, struct archive *a, unsigned int len) {
return 0;
}
#endif

int advance_fragment_cursor(struct fragment_cursor *f, struct archive *a) {
unsigned int metablockindex;
unsigned char buffer8[8];

f->fragindex+=1;
if (f->fragindex>=a->superblock.fragment_entry_count) GOTOERROR;
metablockindex=f->fragindex/512;
if (metablockindex!=f->metablockindex) {
	f->metablockindex=metablockindex;
	if (a->reader.readoff(a->reader.opt,buffer8,a->superblock.fragment_table_start+8*f->metablockindex,8)) GOTOERROR;
	f->metablockoffset=getu64(buffer8);
}
if (loadfragment(f,a,0)) GOTOERROR;
return 0;
error:
	return -1;
}
