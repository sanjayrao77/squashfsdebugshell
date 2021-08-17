#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>
#include <zlib.h>
#define DEBUG
#include "common/conventions.h"

#include "misc.h"

int safeprint(unsigned char *bytes, unsigned int len, FILE *dest) {
while (len) {
	if (isprint(*bytes)) fputc(*bytes,dest);
	else fputc('?',dest);
	bytes++;
	len--;
}
if (ferror(dest)) GOTOERROR;
return 0;
error:
	return -1;
}
