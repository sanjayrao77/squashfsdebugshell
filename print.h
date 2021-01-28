/*
 * print.h
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

int print_superblock(struct superblock *s);
int print_common_inode(struct archive *a, struct common_inode *c, unsigned int indent);
int print_inode(struct archive *a, struct inode *i);
int print_header_dirent(struct header_dirent *h);
int oneline_print_header_dirent(struct header_dirent *h);
int print_header_fragment(struct header_fragment *h);
int print_entry_dirent(struct entry_dirent *e, struct header_dirent *h, char *filename);
int print_combined_dirent(struct archive *a, struct header_dirent *h, struct entry_dirent *e, char *filename);
int print_pointer_metablock(struct pointer_metablock *ptr);
