#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"
#include "filesys.h"
#include "paging.h"

#define MAX_CMD_LEN             128
#define MAX_ARG_LEN             128
#define NUM_PROCESS             6
#define CHECK_BUFFER_SIZE       4
#define NO_PARENT_PID           NUM_PROCESS
/* file descriptor related */
#define MAX_FILE_NUM            8
#define FDA_FILE_START_IDX      2
#define FD_STDIN_IDX            0
#define FD_STDOUT_IDX           1
#define FD_FLAG_FREE            0
#define FD_FLAG_BUSY            1
/* paging & address related */
#define KS_SIZE                 8192
#define KS_BASE_ADDR            0x800000
#define USER_MEM_ADDR           0x8000000
#define PROGRAM_VIRTUAL_ADDR    0x8048000
#define PROGRAM_START_OFFSET    24
#define PROGRAM_START_ADDR      (PROGRAM_VIRTUAL_ADDR + PROGRAM_START_OFFSET)
/* sizeof(int32_t) here is because we need to points to the least position of the user stack, not the bottom */
#define USER_STACK_ADDR         (USER_MEM_ADDR + PAGE_4MB_SIZE - sizeof(int32_t))
/* halt status code */
#define HALT_SPECIAL            0   /* for those already have prompt */
#define HALT_EXCEPTION          1
#define HALT_ABNORMAL           2
#define HALT_EXCEPTION_RETVAL   256

typedef struct file_op_table_t {
    int32_t (*open)  (const char* fname);
    int32_t (*close) (int32_t fd);
    int32_t (*read)  (int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write) (int32_t fd, void* buf, int32_t nbytes);
} file_op_table_t;

typedef struct file_desc_t {
    file_op_table_t* op;    /* file operator table */
    uint32_t inode_idx;     /* inode index */
    uint32_t file_offset;   /* offset in current file */
    uint32_t flags;         /* whether this file descriptor is used */
} file_desc_t;

typedef struct pcb_t {
    /* file descriptor array */
    file_desc_t fd_array[MAX_FILE_NUM];
    /* process id */
    uint32_t pid;
    uint32_t parent_pid;
    /* terminal id */
    uint32_t term_id;
    /* arguments for this process */
    uint8_t arg[MAX_ARG_LEN];
    /* used for context switch */
    uint32_t ebp;
    uint32_t esp;
} pcb_t;

/* current process id */
uint32_t curr_pid;

/* pointer pointing to current fd array */
file_desc_t* cur_fd_array;

/* system call execute, attempts to load and execute a new program, */
/* handing off the processor to the new program until it terminates.*/
int32_t execute(const uint8_t* cmd);

/* terminates a process, returning the specified value to its parent process */
int32_t halt(uint8_t status);

/* system call open, would call particular device's open function according to the file type */
int32_t open(const char* fname);

/* system call close, would call particular device's close function according to the file type */
int32_t close(int32_t fd);

/* system call read, would call particular device's read function according to the file type */
int32_t read(int32_t fd, void* buf, int32_t nbytes);

/* system call write, would call particular device's write function according to the file type */
int32_t write(int32_t fd, void* buf, int32_t nbytes);

/* get args from command and copy it to buffer */
int32_t getargs(uint8_t *buf, int32_t nbytes);

/* maps user space virtual vidmem to physical video memory  */
int32_t vidmap(uint8_t** screen_start);

/* remaps user space virtual vidmem to a physical address */
int32_t vid_remap(uint8_t* phys_addr);

/* get new process id by finding unoccupied position of pid_array */
uint32_t get_new_pid();

/* remaps user space virtual vidmem to a physical address */
inline pcb_t* get_pcb_ptr(uint32_t pid);

/* initialize file operation table array */
void file_op_table_init();

#endif
