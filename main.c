/*
 * main.c - program entry and main loop
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
#include "metablock.h"
#include "archive.h"
#include "directory.h"
#include "print.h"
#include "fileio.h"
#include "nbdclient.h"
#ifdef HAVETLS
#include "nbdtlsclient.h"
#endif
#include "unionio.h"

SICLEARFUNC(archive);
SICLEARFUNC(unionio);

static inline void strip(char *line) {
char *temp;
temp=strchr(line,'\n');
if (temp) *temp=0;
}

int main(int argc, char **argv) {
struct archive archive;
struct unionio unionio;
char *url="-h";

clear_archive(&archive);
clear_unionio(&unionio);

if (argc>1) url=argv[1];
// else url="file://example.fs";
// else url="nbd://192.168.1.7:10809/example";
if (!strcmp(url,"-h")) {
	fprintf(stdout,"Usage: squashfs-shell file://filename\n");
	fprintf(stdout,"Usage: squashfs-shell nbd://1.2.3.4:port/path\n");
#ifdef HAVETLS
	fprintf(stdout,"Usage: squashfs-shell nbdtls://1.2.3.4:port/path\n");
#endif
	return 0;
}

if (init_unionio(&unionio,url)) GOTOERROR;
if (init_archive(&archive,unionio.readoff,unionio.readoffopt)) GOTOERROR;
#if 0
print_superblock(&archive.superblock);
#endif
if (ref_loadinode_archive(&archive,archive.superblock.root_inode_ref)) GOTOERROR;
(void)setrootdirectory_archive(&archive);
fprintf(stderr,"Commands: superblock, inode, id X, ls, l, s, cd X, cat X, cd, metablocks, fragment, header, entry, quit\n");
while (1) {
	char oneline[256];
	fprintf(stderr,"%s/ $ ",archive.pwd.pwdname);
	if (!fgets(oneline,256,stdin)) break;
	strip(oneline);
	if (!strcmp(oneline,"superblock")) {
		print_superblock(&archive.superblock);
	} else if (!strcmp(oneline,"inode")) {
		print_inode(&archive,&archive.inode);
	} else if (!strncmp(oneline,"id ",3)) {
		unsigned int id;
		if (fetch_id(&id,&archive,(unsigned short)atoi(oneline+3))) GOTOERROR;
		printf("id: %u\n",id);
	} else if (!strcmp(oneline,"ls")) {
		int isdone;
		if (loadinode_archive(&archive,&archive.pwd.dirlist[archive.pwd.depth].pointer)) GOTOERROR;
		if (list_directory(&archive,&archive.inode)) GOTOERROR;
		do { if (step_list_directory(&isdone,&archive,0)) GOTOERROR; } while (!isdone);
	} else if (!strcmp(oneline,"l")) {
		if (loadinode_archive(&archive,&archive.pwd.dirlist[archive.pwd.depth].pointer)) GOTOERROR;
		if (list_directory(&archive,&archive.inode)) GOTOERROR;
	} else if (!strcmp(oneline,"s")) {
		int isdone;
		if (step_list_directory(&isdone,&archive,1)) GOTOERROR;
	} else if (!strncmp(oneline,"cd ",3)) {
		if (chdir_directory(&archive,oneline+3)) GOTOERROR;
	} else if (!strncmp(oneline,"cat ",4)) {
		if (catfile_directory(&archive,oneline+4)) GOTOERROR;
	} else if (!strcmp(oneline,"cd")) {
		(void)setrootdirectory_archive(&archive);
	} else if (!strcmp(oneline,"metablocks")) {
		printlist_metablock(archive.metablocks.first);
	} else if (!strcmp(oneline,"fragment")) {
		print_header_fragment(&archive.fragment.header);
	} else if (!strcmp(oneline,"header")) {
		print_header_dirent(&archive.directory.header);
	} else if (!strcmp(oneline,"entry")) {
		print_entry_dirent(&archive.directory.entry,&archive.directory.header,archive.filenamebuffer);
	} else if (!strcmp(oneline,"quit")) break;
	else if (!strcmp(oneline,"q")) break;
	else if (!strcmp(oneline,".")) break;
	else {
		fprintf(stderr,"Unknown command\n");
	}

}

if (shutdown_unionio(&unionio)) GOTOERROR;
deinit_archive(&archive);
deinit_unionio(&unionio);
return 0;
error:
	deinit_archive(&archive);
	deinit_unionio(&unionio);
	return -1;
}
