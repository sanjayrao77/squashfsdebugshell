/*
 * metablock.h
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

struct pointer_metablock {
	unsigned int blocksoffset;
	unsigned short offsetinblock;
};

struct cache_metablock {
	unsigned char *uncompressed;
	uint64_t thisblocksoffset,nextblocksoffset; // full offset from start of archive
	unsigned int usecounter; // number of active cursors
	struct {
		signed char balance;
		struct cache_metablock *left,*right;
	} treevars;
	struct {
		struct cache_metablock **ppnext,*next;
	} listvars;
};

struct cache_metablock *find_cache_metablock(struct cache_metablock *root, uint64_t offset);
void addnode_cache_metablock(struct cache_metablock **treetop_inout, struct cache_metablock *node);
void rmnode_cache_metablock(struct cache_metablock **treetop_inout, struct cache_metablock *node);
int printlist_metablock(struct cache_metablock *c);
