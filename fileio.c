/*
 * fileio.c - local io for abstraction
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
#define _LARGEFILE64_SOURCE
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
#include <zlib.h>
#define DEBUG
#include "common/conventions.h"

#include "fileio.h"

void clear_fileio(struct fileio *f) {
static struct fileio blank={.fd=-1};
*f=blank;
}

int init_fileio(struct fileio *f, char *filename) {
if (-1==(f->fd=open(filename,O_RDONLY))) GOTOERROR;
return 0;
error:
	return -1;
}

static inline int readn(int fd, unsigned char *buff, unsigned int n) {
if (n) while (1) {
	int k;
	k=read(fd,buff,n);
	if (k<=0) GOTOERROR;
	n-=k;
	if (!n) break;
	buff+=k;
}
return 0;
error:
	return -1;
}

static inline int readoff(int fd, unsigned char *dest, uint64_t offset, unsigned int count) {
if (0>lseek64(fd,offset,SEEK_SET)) GOTOERROR;
return readn(fd,dest,count);
error:
	return -1;
}



int readoff_fileio(void *opts, unsigned char *dest, uint64_t offset, unsigned int count) {
struct fileio *f=(struct fileio *)opts;
return readoff(f->fd,dest,offset,count);
}

void deinit_fileio(struct fileio *f) {
ifclose(f->fd);
}

int shutdown_fileio(struct fileio *f) {
int ret;
if (f->fd<0) return 0;
ret=close(f->fd);
f->fd=-1;
return ret;
}
