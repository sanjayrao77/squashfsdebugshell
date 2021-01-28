/*
 * archive.h
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

struct gzip_compressoroptions {
	uint32_t compression_level; // 1..9
	uint16_t window_size; // 8..15
	uint16_t strategies; // flags:1=default,2=filtered,4=huffman,8=rle,16=fixed, 0=>1
};
#define SIZE_GZIP_COMPRESSOROPTIONS	8

#define GZIP_COMPRESSION_ID_SUPERBLOCK	1
#define COMPRESSOROPTIONS_FLAG_SUPERBLOCK 0x0400

struct superblock {
	uint32_t magic;
	uint32_t inode_count;
	uint32_t modification_time;
	uint32_t block_size;
	uint32_t fragment_entry_count;
	uint16_t compression_id;
	uint16_t block_log;
	uint16_t flags;
	uint16_t id_count;
	uint16_t version_major;
	uint16_t version_minor;
	uint64_t root_inode_ref;
	uint64_t bytes_used;
	uint64_t id_table_start;
	uint64_t attr_id_table_start;
	uint64_t inode_table_start;
	uint64_t directory_table_start;
	uint64_t fragment_table_start;
	uint64_t export_table_start;
};
#define SIZE_SUPERBLOCK	96

#define BASICDIRECTORY_TYPE 1
#define BASICFILE_TYPE 2
#define BASICSYMLINK_TYPE 3
#define BASICBDEVICE_TYPE 4
#define BASICCDEVICE_TYPE 5
#define BASICNAMEDPIPE_TYPE 6
#define BASICSOCKET_TYPE 7
#define EXTENDEDDIRECTORY_TYPE 8
#define EXTENDEDFILE_TYPE 9
#define EXTENDEDSYMLINK_TYPE 10
#define EXTENDEDBDEVICE_TYPE 11
#define EXTENDEDCDEVICE_TYPE 12
#define EXTENDEDNAMEDPIPE_TYPE 13
#define EXTENDEDSOCKET_TYPE 14

struct common_inode {
	uint16_t type;
	uint16_t permissions;
	uint16_t uidindex;
	uint16_t gidindex;
	uint32_t mtime;
	uint32_t inode_number;
};
#define SIZE_COMMON_INODE	16

struct basicdirectory_inode  {
	uint32_t block_index;
	uint32_t link_count;
	uint16_t file_size;
	uint16_t block_offset;
	uint32_t parent_inode;
};
#define SIZE_BASICDIRECTORY_INODE	16

struct extendeddirectory_inode  {
	uint32_t link_count;
	uint32_t file_size;
	uint32_t block_index;
	uint32_t parent_inode;
	uint16_t index_count;
	uint16_t block_offset;
	uint32_t xattr_index;
};
#define SIZE_EXTENDEDDIRECTORY_INODE	24

struct basicfile_inode {
	uint32_t blocks_start;
	uint32_t frag_index;
	uint32_t block_offset;
	uint32_t file_size;
// u32[] blocksizes
};
#define SIZE_BASICFILE_INODE	16

struct extendedfile_inode {
	uint64_t blocks_start;
	uint64_t file_size;
	uint64_t sparse;
	uint32_t link_count;
	uint32_t frag_index;
	uint32_t block_offset;
	uint32_t xattr_index;
// u32[] blocksizes
};
#define SIZE_EXTENDEDFILE_INODE	40

struct basicsymlink_inode {
	uint32_t link_count;
	uint32_t target_size;
// u8[] target_path
};
#define SIZE_BASICSYMLINK_INODE	8

struct extendedsymlink_inode {
	uint32_t link_count;
	uint32_t target_size;
// u8[] target_path
	uint32_t xattr_index;
};
#define SIZE_EXTENDEDSYMLINK_INODE	12

struct basicdevice_inode {
	uint32_t link_count;
	uint32_t device_number; // minor12:major12:minor8
};
#define SIZE_BASICDEVICE_INODE	8

struct extendeddevice_inode {
	uint32_t link_count;
	uint32_t device_number; // minor12:major12:minor8
	uint32_t xattr_index;
};
#define SIZE_EXTENDEDDEVICE_INODE	12

struct basicspecial_inode {
	uint32_t link_count;
};
#define SIZE_BASICSPECIAL_INODE	4

struct extendedspecial_inode {
	uint32_t link_count;
	uint32_t xattr_index;
};
#define SIZE_EXTENDEDSPECIAL_INODE	8

struct header_dirent {
	uint32_t countm1;
	uint32_t start;
	uint32_t inode_number; // basis // why does format.txt have this signed?
};
#define SIZE_HEADER_DIRENT	12

struct entry_dirent {
	uint16_t offset;
	int16_t inode_offset; // delta, signed
	uint16_t type;
	uint16_t name_sizem1;
// u8[] name
};
#define SIZE_ENTRY_DIRENT	8

struct header_fragment {
	uint64_t start;
	uint32_t size; // bit24 is uncompressed flag
	uint32_t unused;
};
#define SIZE_HEADER_FRAGMENT	16

#define MAXSIZE_STRUCT (SIZE_COMMON_INODE+SIZE_EXTENDEDFILE_INODE)
#define MAXSIZE_FILENAME	256
#define MAXDEPTH_DIRECTORY	20

struct inode {
	struct common_inode common;
	struct pointer_metablock pointer;
	unsigned short type; // dupe of common.type
	union {
		struct basicdirectory_inode basicdirectory;
		struct extendeddirectory_inode extendeddirectory;
		struct basicfile_inode basicfile;
		struct extendedfile_inode extendedfile;
		struct basicsymlink_inode basicsymlink;
		struct extendedsymlink_inode extendedsymlink;
		struct basicdevice_inode basicbdevice;
		struct extendeddevice_inode extendedbdevice;
		struct basicdevice_inode basiccdevice;
		struct extendeddevice_inode extendedcdevice;
		struct basicspecial_inode basicnamedpipe;
		struct extendedspecial_inode extendednamedpipe;
		struct basicspecial_inode basicsocket;
		struct extendedspecial_inode extendedsocket;
	};
};

struct metablock_cursor {
	struct cache_metablock *cache;
	unsigned int bytesleft;
	unsigned char *cursor;
};

struct fragment_cursor {
	unsigned int fragindex;
	unsigned int metablockindex;
	uint64_t metablockoffset;
	unsigned char *cursor;
	unsigned int offset;
	unsigned int bytesleft;
};

#define MAX_PWDNAME	256
#define MAXDEPTH_DIRLIST	10
struct archive {
	unsigned int dataoffset; // unused ?
	struct {
		unsigned int quotaleft; // if !quotaleft, then start recycling unused metablocks
		struct cache_metablock *treetop;
		struct cache_metablock *first,*last;
	} metablocks;
	struct { // compressed.anyblock/2 should only be used momentarily for decompression or io
		unsigned char *anyblock; // size:max(8192,superblock.block_size)
		unsigned char *anyblock2; // size:same
	} compressed;
	unsigned char structbuffer[MAXSIZE_STRUCT];
	char filenamebuffer[MAXSIZE_FILENAME+1];
	char symlinkbuffer[MAXSIZE_FILENAME+1];
	struct inode inode;
	struct directory {
		struct metablock_cursor cursor;
		unsigned int block_index,file_size;
		unsigned short block_offset;
		unsigned int bytesleft; // unread bytes in record .file_size
		struct header_dirent header;
		unsigned int entry_count; // entries for this header section
		struct entry_dirent entry;
	} directory;
	struct fragment {
		struct header_fragment header;
		unsigned int blocksize; // uncompressed size of fragment <= superblock.block_size
		unsigned char *block; // superblock.block_size
	} fragment;
	struct {
		char pwdname[MAX_PWDNAME+1];
		unsigned int depth;
		struct pointer_directory {
			struct pointer_metablock pointer;
		} dirlist[MAXDEPTH_DIRLIST+1];
	} pwd;
	struct superblock superblock;
	union {
		struct gzip_compressoroptions gzip;
	} compressoroptions;
	struct {
		void *opt;
		int (*readoff)(void *opt,unsigned char *dest, uint64_t offset, unsigned int len);
	} reader;
};

int init_archive(struct archive *a, int (*readoff)(void *, unsigned char *, uint64_t, unsigned int), void *opt);
void deinit_archive(struct archive *a);
char *typetostring_archive(int type, char *unk);
int fetch_id(unsigned int *id_out, struct archive *a, unsigned short idindex);
void reftopointer_archive(struct pointer_metablock *p_out, uint64_t inoderef);
int ref_loadinode_archive(struct archive *a, uint64_t inoderef);
int loadinode_archive(struct archive *a, struct pointer_metablock *p);
void setrootdirectory_archive(struct archive *a);
int isdirectory_archive(unsigned int type);
char typetoletter_archive(int type);
unsigned int major_archive(unsigned int devno);
unsigned int minor_archive(unsigned int devno);
int freadfile_archive(struct archive *a, struct pointer_metablock *p, FILE *fout);
