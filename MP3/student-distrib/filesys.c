#include "lib.h"
#include "filesys.h"
#include "syscall.h"

void* filesys_addr;             /* pointer points to the start of file system */
boot_block_t* boot_block;       /* pointer points to the boot block */
data_block_t* data_block_arr;   /* pointer points to the data block array */
inode_t* inode_arr;             /* pointer points to the inode array */
int cur_dentry_idx;             /* current file dentry index */

/*
 * filesys_init
 * DESCRIPTION: initialize the file system
 * INPUT: filesys -- the address of the start of the filesystem img
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: modify some related status (which will be file descriptor array in the future)
 */
void filesys_init(void* filesys){
    filesys_addr = filesys;
    boot_block = filesys;
    /* make the inode and datablock as arrays for the convenience of accessing */
    inode_arr = &((inode_t*)filesys)[1];
    data_block_arr = &((data_block_t*)filesys)[1+boot_block->inode_num];
    /* init some global variables (which will be file descriptor array in the future) */
    cur_dentry_idx = -1;
}

/*
 * read_dentry_by_name
 * DESCRIPTION: Find dentry with the corresponding filename and copy data through input dentry pointer
 *              Assume that only 32 length file name would not have a '\0' at the end 
 * INPUT: fname -- string of the file name
 *        dentry -- pointer points to a dentry which needs to be filled in
 * OUTPUT: fields of the corresponding dentry
 * RETURN: 0 for success, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry){
    int i;                                       /* iterated index for dentries in boot block */
    int fname_len;                               /* length of filename */
    uint8_t* fname_buf[MAX_FILE_NAME_LEN] = {0}; /* filename buffer for string comparing */
    dentry_t* cur_dentry;                        /* pointer to current dentry */

    /* get the length of the file name */
    fname_len = strlen((int8_t*)fname);

    /* sanity check */
    if(fname == NULL || dentry == NULL || fname_len > MAX_FILE_NAME_LEN)
        return -1;

    /* deal with very long file name */
    if(fname_len == MAX_FILE_NAME_LEN)
        strncpy((int8_t*)fname_buf, (int8_t*)fname, MAX_FILE_NAME_LEN);
    else
        strcpy((int8_t*)fname_buf, (int8_t*)fname);

    /* traverse all dentry until finding the corresponding dentry */
    for(i = 0; i < boot_block->dir_num; i++){
        cur_dentry = &(boot_block->dentry_arr[i]);
        /* compare the file name */
        if(!strncmp((int8_t*)fname_buf, (int8_t*)(cur_dentry->file_name), MAX_FILE_NAME_LEN)){
            /* copy the contents */
            *dentry = *cur_dentry;
            /* success, return 0 */
            return 0;
        }
    }
    /* file not found, return -1 */
    return -1;
}

/*
 * read_dentry_by_index
 * DESCRIPTION: Find dentry with the corresponding index in boot block and 
 *              copy data through input dentry pointer
 * INPUT: idx -- dentry index in boot block
 *        dentry -- pointer points to a dentry which needs to be filled in
 * OUTPUT: fields of the corresponding dentry
 * RETURN: 0 for success, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t read_dentry_by_index(uint32_t idx, dentry_t* dentry){
    /* sanity check */
    if(idx >= boot_block->dir_num)
        return -1;
    /* copy the dentry */
    *dentry = boot_block->dentry_arr[idx];
    /* success */
    return 0;
}

/*
 * read_data
 * DESCRIPTION: Read the data in the file corresponding the the given inode. Read n bytes start from
 *              offset in this file and copy to the buffer.
 * INPUT: inode_idx -- inode index
 *        offset -- byte offset in the file
 *        buf -- buffer needs to be filled in
 *        nbytes -- number of bytes need to be copied
 * OUTPUT: nbytes file data in buf
 * RETURN: number of copied bytes, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t read_data(uint32_t inode_idx, uint32_t offset, uint8_t* buf, uint32_t nbytes){
    int read_bytes;             /* already read bytes                       */
    int cur_block_num;          /* number of block that has been read       */
    int cur_block_idx;          /* index of the current read block          */
    int cur_block_offset;       /* byte offset in the current block         */
    data_block_t* cur_block;    /* pointer points to the current read block */
    inode_t* cur_inode = &(inode_arr[inode_idx]);   /* pointer points to the innode with corresponding index */

    /* sanity check */
    if(buf == NULL || inode_idx >= boot_block->inode_num)
        return -1;

    /* calculate info of current read block */
    cur_block_num = offset/BLOCK_SIZE_BYTE;
    cur_block_idx = cur_inode->data_block_idx[cur_block_num];
    /* sanity check, check whether a bad block index */
    if(cur_block_idx>=boot_block->data_block_num) return -1;
    cur_block = &(data_block_arr[cur_block_idx]);
    cur_block_offset = offset%BLOCK_SIZE_BYTE;

    /* copy data */
    for(read_bytes = 0; read_bytes < nbytes; read_bytes++){
        /* if data is in different block, read from different block */
        if(cur_block_offset >= BLOCK_SIZE_BYTE){
            /* calculate info of current read block */
            cur_block_idx = cur_inode->data_block_idx[++cur_block_num];
            /* sanity check, check whether a bad block index */
            if(cur_block_idx>=boot_block->data_block_num) return -1;
            cur_block = &(data_block_arr[cur_block_idx]);
            cur_block_offset = 0;
        }
        /* if at the end of file, stop reading */
        if(offset++ >= cur_inode->file_size)
            break;
        /* copy a byte */
        *(buf++) = cur_block->data[cur_block_offset++];
    }
    /* return the number of bytes read */
    return read_bytes;
}

/*
 * file_open
 * DESCRIPTION: Open a file with the given filename.
 * INPUT: filename - name of the file which needs to be open
 * OUTPUT: none
 * RETURN: 0 for success, -1 for fail
 * SIDE AFFECTS: global pointers relates to the current file changes
 */
int32_t file_open(const char* filename){
    dentry_t dentry;    /* temp dentry variable */

    /* read dentry by filename & sanity check */
    if(read_dentry_by_name((uint8_t*)filename, &dentry) != 0 || dentry.file_type != FILE_TYPE || dentry.inode_idx >= boot_block->inode_num)
        return -1;

    /* success, return 0 */
    return 0;
}

/*
 * file_close
 * DESCRIPTION: Close the opened file
 * INPUT: fd - file descriptor. But not used.
 * OUTPUT: none
 * RETURN: 0 for success
 * SIDE AFFECTS: global pointers relates to the current file cleared
 */
int32_t file_close(int32_t fd){
    return 0;
}

/*
 * file_read
 * DESCRIPTION: read n bytes from the current opened file
 * INPUT: fd -- file descriptor. But not used.
 *        buf -- buffer to be filled in
 *        nbytes -- number of bytes needed to be copied
 * OUTPUT: none
 * RETURN: number of read bytes, -1 for fail
 * SIDE AFFECTS: global file offset pointers changed
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    int read_bytes; /* number of read bytes */

    /* check whether the file is open */
    if(cur_fd_array[fd].flags == 0)
        return -1;
    /* read data from file */
    if((read_bytes = read_data(cur_fd_array[fd].inode_idx, cur_fd_array[fd].file_offset, buf, nbytes))==-1)
        return -1;
    /* update offset if success */
    cur_fd_array[fd].file_offset += read_bytes;
    /* return the number of bytes read */
    return read_bytes;
}

/*
 * file_write
 * DESCRIPTION: write bytes into the current opened file. Not used.
 * INPUT: fd -- file descriptor. Not used.
 *        buf -- not used.
 *        nbytes -- not used.
 * OUTPUT: none
 * RETURN: -1
 * SIDE AFFECTS: none
 */
int32_t file_write(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}


/*
 * dir_open
 * DESCRIPTION: Open a directory. Initialize the global index of dentry.
 * INPUT: filename -- not used
 * OUTPUT: none
 * RETURN: 0
 * SIDE AFFECTS: global index of dentry is initialized
 */
int32_t dir_open(const char* filename){
    cur_dentry_idx = 0;
    return 0;
}

/*
 * dir_close
 * DESCRIPTION: Close a directory. Not used.
 * INPUT: fd -- file descriptor. Not used.
 * OUTPUT: none
 * RETURN: 0
 * SIDE AFFECTS: none
 */
int32_t dir_close(int32_t fd){
    return 0;
}

/*
 * dir_read
 * DESCRIPTION: Read filename from dentry. Read nbytes of one name for one call and fill the given buffer.
 * INPUT: fd -- file descriptor. Not used.
 *        buf -- buffer to be filled
 *        nbytes -- number of bytes need to be read from current file name
 * OUTPUT: none
 * RETURN: length of read
 * SIDE AFFECTS: none
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    int read_len;       /* length of read file name */
    char* filename;     /* file name string */
    char strbuf[MAX_FILE_NAME_LEN]; 
    int i;              /* loop index in string copying */

    /* sanity check */
    if(buf == NULL || cur_dentry_idx == -1)
        return -1;

    /* if at the end of dentry array, return 0 */
    if(cur_dentry_idx >= boot_block->dir_num)
        return 0;

    /* if the file name is bigger than 32, just copy 32 char */
    read_len = MAX_FILE_NAME_LEN<nbytes?MAX_FILE_NAME_LEN:nbytes;
    filename = boot_block->dentry_arr[cur_dentry_idx++].file_name;

    /* copy the filename to the buffer*/
    for(i = 0; i < read_len; i++)
        strbuf[i] = filename[i];

    strncpy((int8_t*)buf, (int8_t*)strbuf, read_len);

    return read_len;
}

/*
 * dir_write
 * DESCRIPTION: Not used.
 * INPUT: fd -- file descriptor. Not used.
 *        buf -- not used.
 *        nbytes -- not used.
 * OUTPUT: none
 * RETURN: -1
 * SIDE AFFECTS: none
 */
int32_t dir_write(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}

/*
 * get_file_size
 * DESCRIPTION: Get the file size in byte of the given dentry.
 * INPUT: dentry -- pointer points to the corresponding dentry
 * OUTPUT: size of the file corresponding to the given dentry
 * RETURN: size of the file corresponding to the given dentry, 0 for RTC or dir file
 * SIDE AFFECTS: none
 */
uint32_t get_file_size(dentry_t* dentry){
    if(dentry->file_type == FILE_TYPE)
        /* normal file */
        return inode_arr[dentry->inode_idx].file_size;
    else
        /* RTC or dir */
        return 0;
}
