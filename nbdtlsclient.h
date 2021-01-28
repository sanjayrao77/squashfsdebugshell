/*
 * nbdtlsclient.h
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

#define MAX_EXPORTNAME_NBDTLSCLIENT	64
struct nbdtlsclient {
	int fd;
	unsigned char ipv4[4];
	unsigned short port;
	char exportname[MAX_EXPORTNAME_NBDTLSCLIENT+1];
	unsigned int timeout;
	int isno0s:1;
	uint64_t exportsize;
	gnutls_certificate_credentials_t x509_cred;
	gnutls_session_t	tlssession;
};

void clear_nbdtlsclient(struct nbdtlsclient *n);
int init_nbdtlsclient(struct nbdtlsclient *n, unsigned char ipv4_0, unsigned char ipv4_1, unsigned char ipv4_2, unsigned char ipv4_3,
		unsigned short ipv4_port, char *exportname, unsigned int timeout);
void deinit_nbdtlsclient(struct nbdtlsclient *n);
int shutdown_nbdtlsclient(struct nbdtlsclient *n);
int readoff_nbdtlsclient(void *opts, unsigned char *dest, uint64_t offset, unsigned int count);
