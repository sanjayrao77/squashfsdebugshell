/*
 * nbdclient.c - nbd io for abstraction
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
#include <endian.h>
#include <ctype.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <time.h>
#include <errno.h>
#define DEBUG
#include "common/conventions.h"

#include "nbdclient.h"

#define getu16(a) be16toh(*(uint16_t*)(a))
#define getu32(a)	be32toh(*(uint32_t*)(a))
#define getu64(a) be64toh(*(uint64_t*)(a))
#define setu16(a,b) *(uint16_t*)(a)=htobe16(b)
#define setu32(a,b) *(uint32_t*)(a)=htobe32(b)
#define setu64(a,b) *(uint64_t*)(a)=htobe64(b)

static unsigned char dead124[124]; // buffer to hold unused reads

struct handshake_server {
	uint64_t nbdmagic;
	uint64_t ihaveopt;
	uint16_t flags;
};

static inline int timeout_writen(int fd, unsigned char *buff, unsigned int n, time_t maxtime) {
fd_set wset;
if (n) while (1) {
	struct timeval tv;
	time_t t;
	int k;
	FD_ZERO(&wset);
	FD_SET(fd,&wset);
	t=time(NULL);
	if (t>=maxtime) { fprintf(stderr,"Timeout sending to server.\n"); GOTOERROR; }
	tv.tv_sec=maxtime-t;
	tv.tv_usec=0;
	switch (select(fd+1,NULL,&wset,NULL,&tv)) { case 0: continue; case -1: if (errno==EINTR) { sleep(1); continue; } GOTOERROR; }
	k=write(fd,buff,n);
	if (k<=0) GOTOERROR;
	n-=k;
	if (!n) break;
	buff+=k;
}
return 0;
error:
	return -1;
}
static inline int timeout_readn(int fd, unsigned char *buff, unsigned int n, time_t maxtime) {
fd_set rset;
if (n) while (1) {
	struct timeval tv;
	time_t t;
	int k;
	FD_ZERO(&rset);
	FD_SET(fd,&rset);
	t=time(NULL);
	if (t>=maxtime) { fprintf(stderr,"Timeout receiving from server.\n"); GOTOERROR; }
	tv.tv_sec=maxtime-t;
	tv.tv_usec=0;
	switch (select(fd+1,&rset,NULL,NULL,&tv)) { case 0: continue; case -1: if (errno==EINTR) { sleep(1); continue; } GOTOERROR; }
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

void clear_nbdclient(struct nbdclient *n) {
static struct nbdclient blank={.fd=-1};
*n=blank;
}

static inline int nodelay_net(int fd) {
int yesint=1;
return setsockopt(fd,IPPROTO_TCP,TCP_NODELAY, (char*)&yesint,sizeof(int));
}

SICLEARFUNC(sockaddr_in);
static int connecttoserver(struct nbdclient *n, unsigned char ipv4_0, unsigned char ipv4_1, unsigned char ipv4_2, unsigned char ipv4_3,
		unsigned short ipv4_port) {
struct sockaddr_in sa;
socklen_t ssa;
int fd=-1;
fd_set wset;
struct timeval tv;

clear_sockaddr_in(&sa);
sa.sin_family=AF_INET;
((unsigned char *)&sa.sin_addr.s_addr)[0]=ipv4_0;
((unsigned char *)&sa.sin_addr.s_addr)[1]=ipv4_1;
((unsigned char *)&sa.sin_addr.s_addr)[2]=ipv4_2;
((unsigned char *)&sa.sin_addr.s_addr)[3]=ipv4_3;
sa.sin_port=htons(ipv4_port);
if (0>(fd=socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0))) GOTOERROR;
if (nodelay_net(fd)) GOTOERROR;
if (0>connect(fd,(struct sockaddr*)&sa,sizeof(sa))) {
	if (errno!=EINPROGRESS) {
		perror("connect");
		GOTOERROR;
	}
}
FD_ZERO(&wset);
FD_SET(fd,&wset);
tv.tv_sec=n->timeout;
tv.tv_usec=0;
switch (select(fd+1,NULL,&wset,NULL,&tv)) {
	case -1: GOTOERROR;
	case 0: fprintf(stderr,"Timeout waiting for connection.\n"); GOTOERROR;
}
ssa=sizeof(sa);
if (getpeername(fd,(struct sockaddr*)&sa,&ssa)) {
	if (errno==ENOTCONN) GOTOERROR;
	GOTOERROR;
}

n->fd=fd;
return 0;
error:
	ifclose(fd);
	return -1;
}
#define NBDMAGIC 0x4e42444d41474943
#define IHAVEOPT 0x49484156454F5054
// negotiation flags
#define NBD_FLAG_FIXED_NEWSTYLE		(1)
#define NBD_FLAG_NO_ZEROES				(2)
#define NBD_FLAG_C_FIXED_NEWSTYLE	(1)
#define NBD_FLAG_C_NO_ZEROES			(2)
// option requests
#define NBD_OPT_EXPORT_NAME				(1)
#define NBD_OPT_LIST							(3)
#define NBD_OPT_GO								(7)
// option replies
#define NBD_REPLY_MAGIC							(0x3e889045565a9)
#define NBD_REP_ACK									(1)
#define NBD_REP_ERRBIT							(1<<31)
#define NBD_REP_ERR_UNSUP						((1<<31) + 1)
#define NBD_REP_ERR_POLICY					((1<<31) + 2)
#define NBD_REP_ERR_INVALID					((1<<31) + 3)
#define NBD_REP_ERR_PLATFORM				((1<<31) + 4)
#define NBD_REP_ERR_TLS_REQD				((1<<31) + 5)
#define NBD_REP_ERR_UNKNOWN					((1<<31) + 6)
#define NBD_REP_ERR_SHUTDOWN				((1<<31) + 7)
#define NBD_REP_ERR_BLOCK_SIZE_REQD	((1<<31) + 8)
#define NBD_REP_ERR_TOO_BIG					((1<<31) + 9)
// requests
#define NBD_REQUEST_MAGIC					(0x25609513)
// info
#define NBD_INFO_EXPORT							(0)
// transmission flags
#define NBD_FLAG_HAS_FLAGS					(1<<0)
#define NBD_FLAG_READ_ONLY					(1<<1)
#define NBD_FLAG_SEND_FLUSH					(1<<2)
#define NBD_FLAG_SEND_FUA						(1<<3)

#define NBD_FLAG_ROTATIONAL					(1<<4)
#define NBD_FLAG_SEND_TRIM					(1<<5)
#define NBD_FLAG_SEND_WRITE_ZEROES	(1<<6)
#define NBD_FLAG_SEND_DF						(1<<7)

#define NBD_FLAG_CAN_MULTI_CONN			(1<<8)
#define NBD_FLAG_SEND_RESIZE				(1<<9)
#define NBD_FLAG_SEND_CACHE					(1<<10)
#define NBD_FLAG_SEND_FAST_ZERO			(1<<11)
// commands
#define NBD_CMD_READ								(0)
#define NBD_CMD_DISC								(2)
// command flags
#define NBD_CMD_FLAG_FUA						(1<<0)
#define NBD_CMD_FLAG_NO_HOLE				(1<<1)
#define NBD_CMD_FLAG_DF							(1<<2)
#define NBD_CMD_FLAG_REQ_ONE				(1<<3)
#define NBD_CMD_FLAG_FAST_ZERO			(1<<4)
// command reply
#define NBD_SIMPLE_REPLY_MAGIC			(0x67446698)

static char *replytypetostring(unsigned int err, char *def) {
switch (err) {
	case NBD_REP_ACK: return "Ok";
	case NBD_REP_ERR_UNSUP: return "Unsupported";
	case NBD_REP_ERR_POLICY: return "Policy forbids";
	case NBD_REP_ERR_INVALID: return "Invalid";
	case NBD_REP_ERR_PLATFORM: return "Platform doesn't allow";
	case NBD_REP_ERR_TLS_REQD: return "TLS required";
	case NBD_REP_ERR_UNKNOWN:	return "Export not found";
	case NBD_REP_ERR_SHUTDOWN: return "Server is shutting down";
	case NBD_REP_ERR_BLOCK_SIZE_REQD: return "Server needs blocksize";
	case NBD_REP_ERR_TOO_BIG: return "Request too large";
}
return def;
}

static void printflags(FILE *fout, unsigned int flags) {
if (flags& NBD_FLAG_HAS_FLAGS) fputs(" HAS_FLAGS",fout);
if (flags& NBD_FLAG_READ_ONLY) fputs(" READ_ONLY",fout);
if (flags& NBD_FLAG_SEND_FLUSH) fputs(" SEND_FLUSH",fout);
if (flags& NBD_FLAG_SEND_FUA) fputs(" SEND_FUA",fout);

if (flags& NBD_FLAG_ROTATIONAL) fputs(" ROTATIONAL",fout);
if (flags& NBD_FLAG_SEND_TRIM) fputs(" SEND_TRIM",fout);
if (flags& NBD_FLAG_SEND_WRITE_ZEROES) fputs(" SEND_WRITE_ZEROES",fout);
if (flags& NBD_FLAG_SEND_DF) fputs(" SEND_DF",fout);

if (flags& NBD_FLAG_CAN_MULTI_CONN) fputs(" CAN_MULTI_CONN",fout);
if (flags& NBD_FLAG_SEND_RESIZE) fputs(" SEND_RESIZE",fout);
if (flags& NBD_FLAG_SEND_CACHE) fputs(" SEND_CACHE",fout);
if (flags& NBD_FLAG_SEND_FAST_ZERO) fputs(" SEND_FAST_ZERO",fout);
}

static int exportname_negotiate(struct nbdclient *n, time_t maxtime) {
unsigned char buffer[20];
unsigned int exportnamelen;
unsigned int replytype,replylen;
unsigned int tflags;

exportnamelen=strlen(n->exportname);
setu64(buffer,IHAVEOPT);
setu32(buffer+8,NBD_OPT_GO);
setu32(buffer+12,exportnamelen+6);
setu32(buffer+16,exportnamelen);
if (timeout_writen(n->fd,buffer,20,maxtime)) GOTOERROR;
if (exportnamelen) {
	if (timeout_writen(n->fd,(unsigned char *)n->exportname,exportnamelen,maxtime)) GOTOERROR;
}
setu16(buffer,0);
if (timeout_writen(n->fd,buffer,2,maxtime)) GOTOERROR;

if (timeout_readn(n->fd,buffer,20,maxtime)) GOTOERROR;
if (getu64(buffer)!=NBD_REPLY_MAGIC) GOTOERROR;
if (getu32(buffer+8)!=NBD_OPT_GO) GOTOERROR;
replytype=getu32(buffer+12);
replylen=getu32(buffer+16);
if (replytype&NBD_REP_ERRBIT) {
	unsigned int ui;
	fprintf(stderr,"Server sent an error, %u -> %s \"",replytype&~NBD_REP_ERRBIT,replytypetostring(replytype,"Unknown"));
	if (replylen>124) GOTOERROR;
	if (timeout_readn(n->fd,dead124,replylen,maxtime)) GOTOERROR;
	for (ui=0;ui<replylen;ui++) fputc(isprint(dead124[ui])?dead124[ui]:'.',stderr);
	fputs("\"\n",stderr);
	GOTOERROR;
}
if (replytype!=NBD_OPT_LIST) GOTOERROR;
if (replylen!=12) GOTOERROR;
if (timeout_readn(n->fd,buffer,12,maxtime)) GOTOERROR;
if (getu16(buffer)!=NBD_INFO_EXPORT) GOTOERROR;
n->exportsize=getu64(buffer+2);
tflags=getu16(buffer+10);
if (!(tflags&NBD_FLAG_HAS_FLAGS)) GOTOERROR;
// if (tflags&NBD_FLAG_SEND_DF) n->isdfoption=1; // not needed
#if 1
//fprintf(stderr,"Flags: %u 0x%x\n",tflags,tflags);
	fprintf(stdout,"Flags(0x%x):",tflags);
	printflags(stdout,tflags);
	fputs("\n",stdout);
#endif
if (timeout_readn(n->fd,buffer,20,maxtime)) GOTOERROR;
if (getu64(buffer)!=NBD_REPLY_MAGIC) GOTOERROR;
if (getu32(buffer+8)!=NBD_OPT_GO) GOTOERROR;
if (getu32(buffer+12)!=NBD_REP_ACK) GOTOERROR;
if (getu32(buffer+16)!=0) GOTOERROR;
return 0;
error:
	return -1;
}

int init_nbdclient(struct nbdclient *n, unsigned char ipv4_0, unsigned char ipv4_1, unsigned char ipv4_2, unsigned char ipv4_3,
		unsigned short ipv4_port, char *exportname, unsigned int timeout) {
unsigned char buffer[18];
struct handshake_server hs;
unsigned int clientflags;
time_t maxtime;

n->ipv4[0]=ipv4_0;
n->ipv4[1]=ipv4_1;
n->ipv4[2]=ipv4_2;
n->ipv4[3]=ipv4_3;
n->port=ipv4_port;
n->timeout=timeout;
strncpy(n->exportname,exportname,MAX_EXPORTNAME_NBDCLIENT);

if (connecttoserver(n, ipv4_0, ipv4_1, ipv4_2, ipv4_3, ipv4_port)) GOTOERROR;
maxtime=time(NULL)+timeout;

if (timeout_readn(n->fd,buffer,18,maxtime)) GOTOERROR;
hs.nbdmagic=getu64(buffer);
hs.ihaveopt=getu64(buffer+8);
hs.flags=getu16(buffer+16);
if (hs.nbdmagic!=NBDMAGIC) GOTOERROR;
if (hs.ihaveopt!=IHAVEOPT) {
	fprintf(stderr,"Server uses old protocol that isn't supported.\n"); // oldstyle
	GOTOERROR;
}
if (!(hs.flags&NBD_FLAG_FIXED_NEWSTYLE)) {
	fprintf(stderr,"Server uses old protocol that isn't supported.\n"); // unfixed newstyle
	GOTOERROR;
}
if (hs.flags&NBD_FLAG_NO_ZEROES) n->isno0s=1;
clientflags=NBD_FLAG_C_FIXED_NEWSTYLE;
if (n->isno0s) clientflags|=NBD_FLAG_C_NO_ZEROES;
setu32(buffer,clientflags);
if (timeout_writen(n->fd,buffer,4,maxtime)) GOTOERROR;
if (exportname_negotiate(n,maxtime)) GOTOERROR;
#if 0
if (readoff_nbdclient(n,buffer,0,4)) GOTOERROR;
if (memcmp(buffer,"hsqs",4)) GOTOERROR;
#endif
return 0;
error:
	return -1;
}

void deinit_nbdclient(struct nbdclient *n) {
ifclose(n->fd);
}

int shutdown_nbdclient(struct nbdclient *n) {
unsigned char buffer[28];
time_t maxtime;
int ret;

if (n->fd<0) return 0;
maxtime=time(NULL)+n->timeout;
setu32(buffer,NBD_REQUEST_MAGIC);
setu16(buffer+4,0);
setu16(buffer+6,NBD_CMD_DISC);
setu64(buffer+8,1);
setu64(buffer+16,0);
setu32(buffer+24,0);
if (timeout_writen(n->fd,buffer,28,maxtime)) GOTOERROR;
// server does not reply

ret=close(n->fd);
n->fd=-1;
return ret;
error:
	return -1;
}

#define NBD_EPERM (1)
#define NBD_EIO (5)
#define NBD_ENOMEM (12)
#define NBD_EINVAL (22)
#define NBD_ENOSPC (28)
#define NBD_EOVERFLOW (75)
#define NBD_ENOTSUP (95)
#define NBD_ESHUTDOWN (108)

static char *errorvaluetostring(unsigned int errorvalue, char *defstr) {
switch (errorvalue) {
	case 0: return "No error";
	case NBD_EPERM: return "Operation not permitted.";
	case NBD_EIO: return "Input/output error.";
	case NBD_ENOMEM: return "Cannot allocate memory.";
	case NBD_EINVAL: return "Invalid argument.";
	case NBD_ENOSPC: return "No space left on device.";
	case NBD_EOVERFLOW: return "Value too large.";
	case NBD_ENOTSUP: return "Operation not supported.";
	case NBD_ESHUTDOWN: return "Server is in the process of being shut down.";
}
return defstr;
}

int readoff_nbdclient(void *opts, unsigned char *dest, uint64_t offset, unsigned int count) {
struct nbdclient *n=(struct nbdclient *)opts;
unsigned char buffer[28];
time_t maxtime;
unsigned int errorvalue;

if (!count) return 0;
maxtime=time(NULL)+n->timeout;

setu32(buffer,NBD_REQUEST_MAGIC);
// if (n->isdfoption) setu16(buffer+4,NBD_CMD_FLAG_DF);
setu16(buffer+4,0);
setu16(buffer+6,NBD_CMD_READ);
setu64(buffer+8,1);
setu64(buffer+16,offset);
setu32(buffer+24,count);
if (timeout_writen(n->fd,buffer,28,maxtime)) GOTOERROR;
if (timeout_readn(n->fd,buffer,16,maxtime)) GOTOERROR;
if (getu32(buffer)!=NBD_SIMPLE_REPLY_MAGIC) {
	int i;
	for (i=0;i<16;i++) fprintf(stderr,"%2x ",buffer[i]);
	fprintf(stderr,"\n");
	GOTOERROR;
}
errorvalue=getu32(buffer+4);
if (errorvalue) {
	fprintf(stderr,"Server sent an error (READ), %u -> %s",errorvalue,errorvaluetostring(errorvalue,"Unknown"));
	GOTOERROR;
}
if (getu64(buffer+8)!=1) GOTOERROR;
if (timeout_readn(n->fd,dest,count,maxtime)) GOTOERROR;
return 0;
error:
	return -1;
}
