/*
 * unionio.c - io abstraction
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#ifdef HAVETLS
#include <gnutls/gnutls.h>
#endif
#define DEBUG
#include "common/conventions.h"
#include "fileio.h"
#include "nbdclient.h"
#ifdef HAVETLS
#include "nbdtlsclient.h"
#endif

#include "unionio.h"

static unsigned int strtou(char *str) {
unsigned int ret=0;
switch (*str) {
	case '1': ret=1; break;
	case '2': ret=2; break;
	case '3': ret=3; break;
	case '4': ret=4; break;
	case '5': ret=5; break;
	case '6': ret=6; break;
	case '7': ret=7; break;
	case '8': ret=8; break;
	case '9': ret=9; break;
	case '+':
	case '0': break;
	default: return 0; break;
}
while (1) {
	str++;
	switch (*str) {
		case '9': ret=ret*10+9; break;
		case '8': ret=ret*10+8; break;
		case '7': ret=ret*10+7; break;
		case '6': ret=ret*10+6; break;
		case '5': ret=ret*10+5; break;
		case '4': ret=ret*10+4; break;
		case '3': ret=ret*10+3; break;
		case '2': ret=ret*10+2; break;
		case '1': ret=ret*10+1; break;
		case '0': ret=ret*10; break;
		default: return ret; break;
	}
}
return ret;
}


void deinit_unionio(struct unionio *u) {
switch (u->type) {
	case FILEIO_TYPE_UNIONIO:
		deinit_fileio(&u->fileio);
		break;
	case NBDCLIENT_TYPE_UNIONIO:
		deinit_nbdclient(&u->nbdclient);
		break;
#ifdef HAVETLS
	case NBDTLSCLIENT_TYPE_UNIONIO:
		deinit_nbdtlsclient(&u->nbdtlsclient);
		break;
#endif
}
}

int init_unionio(struct unionio *u, char *url) {
if (!strncmp(url,"nbd://",6)) { // nbd://192.168.1.7:10809/example
	char *ipv40,*ipv41,*ipv42,*ipv43,*ipv4p,*export;
	u->type=NBDCLIENT_TYPE_UNIONIO;
	clear_nbdclient(&u->nbdclient);
	ipv40=url+6;
	if (!(ipv41=strchr(ipv40,'.'))) GOTOERROR;
	ipv41++;
	if (!(ipv42=strchr(ipv41,'.'))) GOTOERROR;
	ipv42++;
	if (!(ipv43=strchr(ipv42,'.'))) GOTOERROR;
	ipv43++;
	ipv4p=strchr(ipv43,':');
	if (ipv4p) ipv4p++; else ipv4p="10809";
	if (!(export=strchr(ipv43,'/'))) GOTOERROR;
	export+=1;
	if (init_nbdclient(&u->nbdclient, strtou(ipv40), strtou(ipv41), strtou(ipv42), strtou(ipv43), strtou(ipv4p),export,30)) GOTOERROR;
	u->readoff=readoff_nbdclient;
	u->readoffopt=&u->nbdclient;
#ifdef HAVETLS
} else if (!strncmp(url,"nbdtls://",9)) { // nbdtls://192.168.1.7:10809/example
	char *ipv40,*ipv41,*ipv42,*ipv43,*ipv4p,*export;
	u->type=NBDTLSCLIENT_TYPE_UNIONIO;
	clear_nbdtlsclient(&u->nbdtlsclient);
	ipv40=url+9;
	if (!(ipv41=strchr(ipv40,'.'))) GOTOERROR;
	ipv41++;
	if (!(ipv42=strchr(ipv41,'.'))) GOTOERROR;
	ipv42++;
	if (!(ipv43=strchr(ipv42,'.'))) GOTOERROR;
	ipv43++;
	ipv4p=strchr(ipv43,':');
	if (ipv4p) ipv4p++; else ipv4p="10809";
	if (!(export=strchr(ipv43,'/'))) GOTOERROR;
	export+=1;
	if (init_nbdtlsclient(&u->nbdtlsclient, strtou(ipv40), strtou(ipv41), strtou(ipv42), strtou(ipv43), strtou(ipv4p),export,30)) GOTOERROR;
	u->readoff=readoff_nbdtlsclient;
	u->readoffopt=&u->nbdtlsclient;
#endif
} else if (!strncmp(url,"file://",7)) {
	u->type=FILEIO_TYPE_UNIONIO;
	clear_fileio(&u->fileio);
	if (init_fileio(&u->fileio,url+7)) GOTOERROR;
	u->readoff=readoff_fileio;
	u->readoffopt=&u->fileio;
} else GOTOERROR;
return 0;
error:
	return -1;
}

int shutdown_unionio(struct unionio *u) {
switch (u->type) {
	case FILEIO_TYPE_UNIONIO: return shutdown_fileio(&u->fileio);
	case NBDCLIENT_TYPE_UNIONIO: return shutdown_nbdclient(&u->nbdclient);
#ifdef HAVETLS
	case NBDTLSCLIENT_TYPE_UNIONIO: return shutdown_nbdtlsclient(&u->nbdtlsclient);
#endif
}
return 0;
}
