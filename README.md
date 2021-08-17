# squashfs debug shell

## Purpose
This is mainly for developers who want to debug squashfs archives. It could
also be used to confirm network connections to remote squashfs archives.

If you are a developer wanting to understand the squashfs structure, this might
be a good base to build from. It's simple and small.

This supports reading local files as well as files shared over NBD.

### Getting started
First, build the program (see below), then try:
```
$ ./shell-tls file://example.fs
Commands: superblock, inode, id X, ls, l, s, cd X, cat X, cd, metablocks, fragment, header, entry, quit
/ $ superblock
struct superblock {
	uint32_t magic			= 1936814952;
	uint32_t inode_count		= 11;
	uint32_t modification_time	= 1585196329;
	uint32_t block_size		= 4096;
	uint32_t fragment_entry_count	= 0;
	uint16_t compression_id		= 1;
	uint16_t block_log		= 12;
	uint16_t flags			= 0x21b;
	uint16_t id_count		= 1;
	uint16_t version_major	= 4, version_minor	= 0;
	uint64_t root_inode_ref		= 348;
	uint64_t bytes_used		= 2477;
	uint64_t id_table_start		= 0x9a5;
	uint64_t attr_id_table_start	= 0xffffffffffffffff;
	uint64_t inode_table_start	= 0x744;
	uint64_t directory_table_start	= 0x8c2;
	uint64_t fragment_table_start	= 0x99f;
	uint64_t export_table_start	= 0xffffffffffffffff;
};
/ $ ls
-rw-r--r--    1 1000 1000              138 Mar 24 19:19 2020 Makefile
drwxr-xr-x    2 1000 1000               45 Mar 25 12:47 2020 governor
drwxr-xr-x    2 1000 1000               45 Mar 23 23:28 2020 magickeyboard
drwxr-xr-x    2 1000 1000               60 Mar 23 23:27 2020 noise
/ $ cat Makefile
all: none
clean:
	( cd magickeyboard ; make clean )
	( cd governor ; make clean )
jesus: clean
	tar -jcf - . | jesus src.rpiutils.tar.bz2

/ $ q
```

### Building
To start, try building it without TLS support:
```bash
	make -f Makefile.notls shell
```
This should make *shell*.

If you have GNU TLS headers and libraries installed:
```bash
	make
```
This should make *shell-tls*.

## Command line
```bash
./shell -h
./shell file://filename.bin
./shell nbd://192.168.1.1:3000/exportname
./shell nbdtls://192.168.1.1:10809/exportname
```
Note that only ipv4-style urls are currently supported.

## Usage
If the shell successfully connects to the archive, you should see a list of commands
and a *$* prompt. This command line is very primitive so don't expect much.
You can enter *quit* to quit the shell.

## Commands
### superblock
*superblock* prints the squashfs superblock.
### inode
*inode* prints the last inode read.
### id X
*id X* fetches the id corresponding with the index *X*. e.g. *id 1*
### ls
*ls* lists the contents of the pwd.
### l
*l* loads the inode of the pwd. You can use the *inode* command after this.
This is necessary before using the *s* command.
### s
*s* presumes *l* has already been run. The first use opens the pwd
and successive calls step through the entries in the directory.
You can repeat this command until the directory has been finished.

You can use the *header* command after the first use to print the directory header
structure.
You can use the *entry* command to print the directory entry structure for
the latest entry.
### cd X
*cd X* changes the pwd to the X directory.
### cat X
*cat X* cats the file X.
### metablocks
*metablocks* prints a list of the metablocks in the cache.
### fragment
*fragment* prints the last fragment read.
### header
*header* prints the last directory header read.
### entry
*entry* prints the last entry header read.
### quit
*quit*, *q* and *.* all quit the shell.
