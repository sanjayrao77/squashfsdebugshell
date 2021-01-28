/*
 * fill.h
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

void common_inode_fill(struct common_inode *c, unsigned char *src);
void basicdirectory_inode_fill(struct basicdirectory_inode *b, unsigned char *src);
void extendeddirectory_inode_fill(struct extendeddirectory_inode *e, unsigned char *src);
void superblock_fill(struct superblock *s, unsigned char *buffer);
void header_dirent_fill(struct header_dirent *h, unsigned char *buffer);
void entry_dirent_fill(struct entry_dirent *e, unsigned char *buffer);
void basicfile_inode_fill(struct basicfile_inode *b, unsigned char *src);
void extendedfile_inode_fill(struct extendedfile_inode *b, unsigned char *src);
void basicsymlink_inode_fill(struct basicsymlink_inode *b, unsigned char *src);
void extendedsymlink_inode_fill(struct extendedsymlink_inode *b, unsigned char *src);
void basicdevice_inode_fill(struct basicdevice_inode *b, unsigned char *src);
void extendeddevice_inode_fill(struct extendeddevice_inode *b, unsigned char *src);
void basicspecial_inode_fill(struct basicspecial_inode *b, unsigned char *src);
void extendedspecial_inode_fill(struct extendedspecial_inode *b, unsigned char *src);
void header_fragment_fill(struct header_fragment *h, unsigned char *src);
