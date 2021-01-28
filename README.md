# squashfs debug shell

## Purpose
This is mainly for developers who want to debug squashfs archives. It could
also be used to confirm network connections to remote squashfs archives.

If you are a developer wanting to understand the squashfs structure, this might
be a good base to build from. It's simple and small.

This supports reading local files as well as files shared over NBD.

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
You can use the *header* command after this to print the directory header
structure.
This is necessary before using the *s* command.
### s
*s* presumes *l* has already been run. This steps through the entries
in the pwd. You can repeat this command until the directory has been finished.
You can use the *entry* command to print the directory entry structure for
the latest entry.
### cd X
*cd X* changes the pwd to the X directory.
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
