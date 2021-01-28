/*
 * unionio.h
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

#define FILEIO_TYPE_UNIONIO				1
#define NBDCLIENT_TYPE_UNIONIO		2
#ifdef HAVETLS
#define NBDTLSCLIENT_TYPE_UNIONIO	3
#endif

struct unionio {
	int type;
	union {
		struct fileio fileio;
		struct nbdclient nbdclient;
#ifdef HAVETLS
		struct nbdtlsclient nbdtlsclient;
#endif
	};
	int (*readoff)(void *,unsigned char *,uint64_t,unsigned int);
	void *readoffopt;
};

void deinit_unionio(struct unionio *u);
int init_unionio(struct unionio *u, char *url);
int shutdown_unionio(struct unionio *u);
