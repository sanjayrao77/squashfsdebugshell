/*
 * directory.h
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

int list_directory(struct archive *a, struct inode *inode);
int step_list_directory(int *isdone_out, struct archive *a, int isverbose);
int back_directory(struct archive *a);
int same_directory(struct archive *a);
int chdir_directory(struct archive *a, char *dirname);
int enter_directory(struct archive *a, char *dirname);
int catfile_directory(struct archive *a, char *filename);
