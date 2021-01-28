
#if 0
static int readgzipoptions(struct archive *a, int fd) {
	unsigned char buffer[SIZE_GZIP_COMPRESSOROPTIONS];
	struct gzip_compressoroptions *g_cc;
	int strategy=Z_DEFAULT_STRATEGY;
	g_cc=&a->compressoroptions.gzip;
	if (readn(fd,buffer,SIZE_GZIP_COMPRESSOROPTIONS)) GOTOERROR;
	g_cc->compression_level=getu32(buffer);
	g_cc->window_size=getu16(buffer+2);
	g_cc->strategies=getu16(buffer+4);
	if (g_cc->strategies>1) { // is this ever set? a bitmask converted to a single item, ugh
		if (g_cc->strategies&2) strategy=Z_FILTERED;
		else if (g_cc->strategies&4) strategy=Z_HUFFMAN_ONLY;
		else if (g_cc->strategies&8) strategy=Z_RLE;
		else if (g_cc->strategies&16) strategy=Z_FIXED;
	}
	if (Z_OK!=deflateInit2(x,x,Z_DEFLATED,g_cc->window_size,9,strategy)) GOTOERROR;
return 0;
error:
	return -1;
}
#endif
