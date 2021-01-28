/*
 * cursor.h
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

int init_metablock_cursor(struct metablock_cursor *c, struct archive *a, uint64_t fulloffset, unsigned int offsetinblock);
void deinit_metablock_cursor(struct metablock_cursor *c);
int advance_metablock_cursor(struct metablock_cursor *c, struct archive *a);
int readbytes_metablock_cursor(unsigned char *dest, struct metablock_cursor *c, struct archive *a, unsigned int len);

int init_fragment_cursor(struct fragment_cursor *f, struct archive *a, unsigned int fragindex, unsigned int offsetinfragment);
#define deinit_fragment_cursor(a) do{}while(0)
int readbytes_fragment_cursor(unsigned char *dest, struct fragment_cursor *f, struct archive *a, unsigned int len);
int advance_fragment_cursor(struct fragment_cursor *fc, struct archive *a);
