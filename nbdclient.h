/*
 * nbdclient.h
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

#define MAX_EXPORTNAME_NBDCLIENT	64
struct nbdclient {
	int fd;
	unsigned char ipv4[4];
	unsigned short port;
	char exportname[MAX_EXPORTNAME_NBDCLIENT+1];
	unsigned int timeout;
	int isno0s:1;
	uint64_t exportsize;
};

void clear_nbdclient(struct nbdclient *n);
int init_nbdclient(struct nbdclient *n, unsigned char ipv4_0, unsigned char ipv4_1, unsigned char ipv4_2, unsigned char ipv4_3,
		unsigned short ipv4_port, char *exportname, unsigned int timeout);
void deinit_nbdclient(struct nbdclient *n);
int shutdown_nbdclient(struct nbdclient *n);
int readoff_nbdclient(void *opts, unsigned char *dest, uint64_t offset, unsigned int count);
