CFLAGS=-g -Wall -O2 -DHAVETLS
# CFLAGS=-g -Wall -O2
all: shell-tls
shell-tls: main.o metablock.o archive.o fill.o print.o cursor.o fileio.o directory.o nbdclient.o nbdtlsclient.o unionio.o misc.o
	gcc -o $@ $^ -lz -lgnutls
shell: main.o metablock.o archive.o fill.o print.o cursor.o fileio.o directory.o nbdclient.o unionio.o misc.o
	gcc -o $@ $^ -lz
clean:
	rm -f *.o shell shell-tls core
jesus: clean
	tar -jcf - . | jesus src.squashfs.tar.bz2
.PHONY: clean jesus
