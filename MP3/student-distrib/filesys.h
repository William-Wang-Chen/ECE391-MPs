#ifndef _FILESYS_H
#define _FILESYS_H

#include "types.h"

#define BLOCK_SIZE_BYTE             4096
#define MAX_FILE_NAME_LEN           32
#define DENTRY_RESERVED_BYTE        24
#define BOOT_BLOCK_RESERVED_BYTE    52
#define MAX_DENTRY_NUM              (BLOCK_SIZE_BYTE-64)/64
#define MAX_INODE_DATA_BLOCK_NUM    (BLOCK_SIZE_BYTE-4)/4

#define FILE_TYPE_NUM   4
#define RTC_TYPE        0
#define DIR_TYPE        1
#define FILE_TYPE       2
#define STD_TYPE        3

typedef struct dentry_t{
    char        file_name[MAX_FILE_NAME_LEN];
    uint32_t    file_type;
    uint32_t    inode_idx;
    uint8_t     reserved[DENTRY_RESERVED_BYTE];
} dentry_t;

typedef struct boot_block_t{
    uint32_t    dir_num;
    uint32_t    inode_num;
    uint32_t    data_block_num;
    uint8_t     reserved[BOOT_BLOCK_RESERVED_BYTE];
    dentry_t    dentry_arr[MAX_DENTRY_NUM];
} boot_block_t;

typedef struct inode_t{
    uint32_t    file_size;  // file length in Byte
    uint32_t    data_block_idx[MAX_INODE_DATA_BLOCK_NUM];
} inode_t;

typedef struct data_block_t{
    uint8_t     data[BLOCK_SIZE_BYTE];
} data_block_t;

/* initialize the file system */
extern void filesys_init(void* filesys);
/* read dentry with the corresponding filename */
extern int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
/* read entry with the corresponding index in boot block */
extern int32_t read_dentry_by_index(uint32_t idx, dentry_t* dentry);
/* Read the data in the file corresponding the the given inode. */
extern int32_t read_data(uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t nbytes);

/* Open a file with the given filename. */
extern int32_t file_open(const char* filename);
/* Close the opened file */
extern int32_t file_close(int32_t fd);
/* Read n bytes from the current opened file */
extern int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
/* Write bytes into the current opened file. Not used. */
extern int32_t file_write(int32_t fd, void* buf, int32_t nbytes);

/* Open a directory. Initialize the global index of dentry. */
extern int32_t dir_open(const char* filename);
/* Close a directory. Not used. */
extern int32_t dir_close(int32_t fd);
/* Read filename from dentry. Read nbytes of one name for one call and fill the given buffer. */
extern int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
/* Not used. */
extern int32_t dir_write(int32_t fd, void* buf, int32_t nbytes);

/* Get the file size in byte of the given dentry. */
extern uint32_t get_file_size(dentry_t* dentry);

#endif
