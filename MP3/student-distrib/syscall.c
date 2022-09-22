#include "syscall.h"
#include "lib.h"
#include "paging.h"
#include "x86_desc.h"
#include "rtc.h"
#include "filesys.h"
#include "terminal.h"

/* file operation table array */
static file_op_table_t file_op_table_arr[FILE_TYPE_NUM];
/* process id array */
static uint32_t pid_array[NUM_PROCESS] = {0};

/*
 * halt
 * DESCRIPTION: terminates a process, returning the specified value to its parent process
 * INPUT: status - halt status
 * OUTPUT: different halt return value to indicate halt status
 * RETURN: 256 if halt by exception, otherwise status
 * SIDE AFFECTS: context switch & PCB cleared & current file descriptor array changed
 */
int32_t halt(uint8_t status)
{
    int fd;                             /* file descriptor array index */
    int curr_process_term_id;           /* current running process' terminal id */
    uint16_t retval;                    /* return value */
    pcb_t *curr_pcb, *parent_pcb;       /* pcb pointers */

    /* forbid interrupt */
    cli();

    /* get current process' pcb pointer */
    curr_pcb = get_pcb_ptr(curr_pid);

    /* get current process' terminal id */
    curr_process_term_id = curr_pcb->term_id;

    /* clear pid */
    pid_array[curr_pcb->pid] = 0;

    /* get parent pcb, if current process is the base shell, just load itsself as its parent for re-executing */
    parent_pcb = get_pcb_ptr((curr_pcb->parent_pid == NO_PARENT_PID) ? curr_pid : curr_pcb->parent_pid);

    /* clear fd array, close any relevant files */
    for(fd = FDA_FILE_START_IDX; fd < MAX_FILE_NUM; fd++){
        if(cur_fd_array[fd].flags)
            close(fd);
    }
    /* clear stdin fd */
    cur_fd_array[0].op = NULL;
    cur_fd_array[0].flags = FD_FLAG_FREE;
    /* clear stdout fd */
    cur_fd_array[1].op = NULL;
    cur_fd_array[1].flags = FD_FLAG_FREE;

    /* restore parent fd array */
    cur_fd_array = parent_pcb->fd_array;

    /* restore parent paging */
    set_paging(parent_pcb->pid);

    /* restore tss data, i.e. kernel stack pointer */
    tss.esp0 = KS_BASE_ADDR - KS_SIZE*parent_pcb->pid - sizeof(int32_t);

    /* update terminal info */
    terminals[curr_process_term_id].pnum--;

    /* if it is the base shell, restart it */
    if(curr_pcb->parent_pid == NO_PARENT_PID){
        clear();
        sti();
        execute((uint8_t*)"shell");
    }

    /* update pid */
    curr_pid = parent_pcb->pid;

    /* update terminal info */
    terminals[curr_process_term_id].curr_pid = curr_pid;

    /* decide return value according to the halt status */
    retval = (status == HALT_EXCEPTION) ? HALT_EXCEPTION_RETVAL : (uint16_t)status;

    /* add an line break to fix a small deficiency of the shell program */
    if(parent_pcb->term_id == curr_term_id)
        putc('\n');
    else
        terminal_putc('\n');

    /* halt and enable interrupt */
    asm volatile("              \n\
        movl    %0, %%esp       \n\
        movl    %1, %%ebp       \n\
        xorl    %%eax, %%eax    \n\
        movw    %2, %%ax        \n\
        sti                     \n\
        leave                   \n\
        ret                     \n\
        "
        : 
        : "r" (parent_pcb->esp), "r"(parent_pcb->ebp), "r"(retval)
        : "esp", "ebp", "eax"
    );

    /* never reach here */
    return -1;
}

/*
 * execute
 * DESCRIPTION: system call execute, attempts to load and execute a new program, 
 *              handing off the processor to the new program until it terminates.
 * INPUT: cmd -- pointer pointes to the command string
 * OUTPUT: none
 * RETURN: 0 for success, -1 for fail
 * SIDE AFFECTS: context switch & PCB added & current file descriptor array changed
 */
int32_t execute(const uint8_t *cmd)
{
    /* parsed command and argument */
    uint8_t command[MAX_CMD_LEN];
    uint8_t argument[MAX_ARG_LEN];
    /* file excitability check buffer */
    uint8_t check_buffer[CHECK_BUFFER_SIZE];
    /* loop index */
    int i;
    /* start, end of the cmd and arg, used in parser */
    int start, end;
    /* check dentry in executable check */  
    dentry_t check_dentry;
    /* new process id */
    uint32_t new_pid;
    /* pcb pointer */
    pcb_t *curr_pcb, *new_pcb;
    /* EIP and ESP setting */
    uint32_t new_eip, new_esp;

    /* forbid interrupt */
    cli();

    /* ================================= *
     * 1. parse the command and argument *
     * ================================= */

    /* start from position 0 */
    start = 0;
    /* until a non-space char appears */
    while (' ' == cmd[start])   start++;
    /* parse the commands */
    for (i = start; i < strlen((int8_t*)cmd); i++)
    {
        /* if the commands is too long, report an error */
        if(i - start > MAX_CMD_LEN - 1)
        {
            sti();
            return -1;
        }
        /* commands ends if meet a empty char */
        if(cmd[i] == ' ')   break;
        /* store the parsed cmd into a buffer */
        command[i-start] = cmd[i];
    }
    /* end of the command */
    command[i-start] = '\0';
    /* skip all the empty char between command and argument */
    start = i;
    while (' ' == cmd[start]) start++;
    /* get the length of argument */
    end = start;
    while (cmd[end] != '\0' && cmd[end] != ' ' && cmd[end] != '\n') end++;
    /* also stores the argument into a buffer */
    for (i = start; i < end; i++)
        argument[i-start] = cmd[i];
    /* end of the argument */
    argument[i-start] = '\0';

    /* =========================== *
     * 2. check file executability *
     * =========================== */

    /* is a file in the fs? */
    if(0 != read_dentry_by_name(command, &check_dentry)){
        sti();
        return -1;
    }

    /* is valid exectuable? */
    /* check file type */
    if(check_dentry.file_type != FILE_TYPE){
        sti();
        return -1;
    }
    /* read magic number of the excutable file */
    read_data(check_dentry.inode_idx, 0, check_buffer, CHECK_BUFFER_SIZE);
    /* check the magic number of the excutable file: 0x7F, E, L, F */
    if(check_buffer[0]!=0x7f && check_buffer[1]!='E' && check_buffer[2]!='L' && check_buffer[3]!= 'F')
    {
        sti();
        return -1;
    }


    /* ============= *
     * 3. set paging *
     * ============= */

    /* get new process id */
    if ((new_pid = get_new_pid()) != -1)
    {
        set_paging(new_pid);
    }else{
        /* Current number of running process exceeds */
        sti();
        return HALT_SPECIAL;
    }

    /* ==================== *
     * 4. load user program *
     * ==================== */
    if(read_data(check_dentry.inode_idx, 0, (uint8_t*)PROGRAM_VIRTUAL_ADDR, get_file_size(&check_dentry)) == -1){
        sti();
        return -1;
    }

    /* ================= *
     * 5. initialize PCB *
     * ================= */

    /*
     *  pcb struct reference:
     *      typedef struct pcb_t {
     *          file_desc_t fd_array[MAX_FILE_NUM];
     *          uint32_t pid;
     *          uint32_t parent_pid;
     *          uint8_t arg[MAX_ARG_LEN];
     *          uint32_t parent_ebp;
     *          uint32_t parent_esp;
     *          uint32_t subprogram_user_eip;
     *          uint32_t subprogram_user_esp;
     *          uint32_t tss_esp;
     *      } pcb_t; 
     */

    /* get curr and new pcb address */
    new_pcb = get_pcb_ptr(new_pid);
    /* set process id */
    new_pcb->pid = new_pid;
    /* set parent process id and terminal id */
    if(terminals[curr_term_id].pnum == 0){
        /* if it is the base shell */
        new_pcb->parent_pid = NO_PARENT_PID;
        new_pcb->term_id = curr_term_id;
    }else{
        new_pcb->parent_pid = curr_pid;
        new_pcb->term_id = get_pcb_ptr(curr_pid)->term_id;
    }
    
    /* initialize the fd_array */
    /* init all file descriptor */
    for (i = 0; i < MAX_FILE_NUM; i++)
    {
        new_pcb->fd_array[i].op = NULL;
        new_pcb->fd_array[i].inode_idx = -1;
        new_pcb->fd_array[i].file_offset = 0;
        new_pcb->fd_array[i].flags = FD_FLAG_FREE;
    }

    /* init stdin */
    new_pcb->fd_array[0].op = &file_op_table_arr[STD_TYPE];
    new_pcb->fd_array[0].flags = FD_FLAG_BUSY;

    /* init stdout */
    new_pcb->fd_array[1].op = &file_op_table_arr[STD_TYPE];
    new_pcb->fd_array[1].flags = FD_FLAG_BUSY;

    /* set current fd array */
    cur_fd_array = new_pcb->fd_array;

    /* set argument */
    strncpy((int8_t*)new_pcb->arg,(int8_t*)argument, MAX_ARG_LEN);

    /* set kernel stack pointer */
    tss.esp0 = KS_BASE_ADDR - KS_SIZE * new_pid - sizeof(int32_t);

    /* store esp and ebp if it is not the first shell of first kernel */
    /* this esp & ebp can be used for halt when system want to restore parent stack info */
    /* 
     * it can also be used for the first schdueling in the new executed shell 
     * when a new shell in a new terminal is executed, a schedule interrupt would happen
     * and try to restore the stack info of the current program
     * however, the new shell is not entered by scheduler() but the execute() function
     * therefore, the scheduler would let the system restore the execute() stack status
     * and would continue executing scheduler()'s instruction, which is leave & ret
     * so the system would back to terminal_switch() from execute()
     * then return to keyboard function finally return to interrupt linkage code
     * then enable interrupt and return from interrupt linkage code
     */
    if(curr_pid != -1){
        curr_pcb = get_pcb_ptr(curr_pid);
        asm volatile("                                \n\
            movl %%ebp, %0                            \n\
            movl %%esp, %1                            \n\
            "
            : "=r"(curr_pcb->ebp), "=r"(curr_pcb->esp)
        );
    }

    /* update current pid */
    curr_pid = new_pid;

    /* update terminal info */
    terminals[curr_term_id].curr_pid = curr_pid;
    terminals[curr_term_id].pnum++;

    /* ================================ *
     * 6.context switch to user program *
     * ================================ */

    /* set the address of the first instruction */
    new_eip = *(int32_t*)PROGRAM_START_ADDR;
    new_esp = USER_STACK_ADDR;

    /* set infomation for IRET to user program space, enable interrupt */
    asm volatile ("                                                \n\
        movw    %%cx, %%ds                                         \n\
        pushl   %%ecx                                              \n\
        pushl   %%ebx                                              \n\
        pushfl                                                     \n\
        popl    %%ecx                                              \n\
        orl     $0x0200, %%ecx                                     \n\
        pushl   %%ecx                                              \n\
        pushl   %%edx                                              \n\
        pushl   %%eax                                              \n\
        sti                                                        \n\
        iret                                                       \n\
        "
        :
        : "a"(new_eip), "b"(new_esp), "c"(USER_DS), "d"(USER_CS)
        : "memory"
    );

    return 0;
}

/*
 * open
 * DESCRIPTION: system call open, would call particular device's open function according to the file type
 * INPUT: filename - name of the file which needs to be open
 * OUTPUT: none
 * RETURN: fd index for success, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t open(const char *fname)
{
    dentry_t dentry;    /* temp dentry for file information */
    int fd;             /* file descriptor array index */

    /* sanity check */
    if (fname == NULL || cur_fd_array == NULL)
        return -1;

    /* find unused file descriptor */
    for (fd = FDA_FILE_START_IDX; fd < MAX_FILE_NUM; fd++)
    {
        if (cur_fd_array[fd].flags == FD_FLAG_FREE)
            break;
    }

    /* fail if reach the max file number or could not find the file */
    if (fd >= MAX_FILE_NUM || read_dentry_by_name((uint8_t*)fname, &dentry) != 0)
        return -1;

    /* set the file operator table pointer */
    cur_fd_array[fd].op = &file_op_table_arr[dentry.file_type];

    /* call the coorsponding routine, if fail return -1 */
    if ((cur_fd_array[fd].op->open(fname)) == -1)
        return -1;

    /* if success, set the file descriptor */
    cur_fd_array[fd].inode_idx = (dentry.file_type == FILE_TYPE) ? dentry.inode_idx : -1;
    cur_fd_array[fd].file_offset = 0;
    cur_fd_array[fd].flags = FD_FLAG_BUSY;

    /* return file descriptor index */
    return fd;
}

/*
 * close
 * DESCRIPTION: system call close, would call particular device's close function according to the file type
 * INPUT: fd -- file descriptor array index of the file to be closed
 * OUTPUT: none
 * RETURN: 0 index for success, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t close(int32_t fd)
{
    int ret; /* return value of perticular close function */

    /* sanity check */
    if (fd < FDA_FILE_START_IDX || fd > MAX_FILE_NUM || cur_fd_array == NULL || cur_fd_array[fd].flags == FD_FLAG_FREE)
        return -1;

    /* close the file and clear the file descriptor */
    if ((ret = cur_fd_array[fd].op->close(fd)) == 0)
    {
        /* if successfully closed, clear the file descriptor */
        cur_fd_array[fd].op = NULL;
        cur_fd_array[fd].inode_idx = -1;
        cur_fd_array[fd].file_offset = 0;
        cur_fd_array[fd].flags = FD_FLAG_FREE;
    }

    return ret;
}

/*
 * read
 * DESCRIPTION: system call read, would call particular device's read function according to the file type
 * INPUT: fd -- file descriptor array index of the file to be read
 *        buf -- buffer to be filled in (can be not used)
 *        nbytes -- number of bytes needed to be read (can be not used)
 * OUTPUT: none
 * RETURN: 0 index for success, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t read(int32_t fd, void *buf, int32_t nbytes)
{
    /* sanity check */
    if (fd < 0 || fd > MAX_FILE_NUM || fd == FD_STDOUT_IDX || buf == NULL || cur_fd_array == NULL || cur_fd_array[fd].flags == FD_FLAG_FREE || cur_fd_array[fd].op == NULL)
        return -1;

    /* call corresponding read function */
    return cur_fd_array[fd].op->read(fd, buf, nbytes);
}

/*
 * write
 * DESCRIPTION: system call write, would call particular device's write function according to the file type
 * INPUT: fd -- file descriptor array index of the file to be write
 *        buf -- not used
 *        nbytes -- not used
 * OUTPUT: none
 * RETURN: 0 index for success, -1 for fail
 * SIDE AFFECTS: none
 */
int32_t write(int32_t fd, void *buf, int32_t nbytes)
{
    /* sanity check */
    if (fd < 0 || fd > MAX_FILE_NUM || fd == FD_STDIN_IDX || buf == NULL || cur_fd_array == NULL || cur_fd_array[fd].flags == FD_FLAG_FREE || cur_fd_array[fd].op == NULL)
        return -1;

    /* call corresponding write function */
    return cur_fd_array[fd].op->write(fd, buf, nbytes);
}

/* 
 * getargs
 * Description: get args from command and copy it to buffer
 * Input:   buf -- destination buffer's pointer
 *          nbytes -- number of bytes to copy
 * Output: 0 for success, -1 for failure
 * Side Effect: copy args to buffer
 */
int32_t getargs(uint8_t *buf, int32_t nbytes)
{
    /* get current pcb */
    pcb_t* pcb_ptr;
    pcb_ptr = get_pcb_ptr(curr_pid);

    /* sanity check */
    if (buf == NULL || pcb_ptr->arg[0] == '\0')
        return -1;

    /* copy to buffer */
    strncpy((int8_t*)buf, (int8_t*)pcb_ptr->arg, nbytes);

    /* success, return 0 */
    return 0;
}

/* 
 *  vidmap
 *  Description: maps user space virtual vidmem to physical video memory 
 *               and return virtual vidmem address to user
 *  Input:  screen_start -- a pointer points to a place where to output virtual video memory addr for user
 *  Output: 0 for success, -1 for failure, virtual vidmem address
 */
int32_t vidmap(uint8_t** screen_start)
{
    /* check if the pointer is in user space */
    if ((unsigned int)screen_start <= ADDR_128MB || (unsigned int)screen_start >= ADDR_132MB)
        return -1;

    /* output vidmem virtual address for user */
    *screen_start = (uint8_t*)VID_VIRTUAL_ADDR;

    /* initialize the VIDMAP page */
    page_directory[VIDMAP_OFFSET].p           = 1;    // present
    page_directory[VIDMAP_OFFSET].r_w         = 1;    // enable r/w
    page_directory[VIDMAP_OFFSET].u_s         = 1;    // user mode
    page_directory[VIDMAP_OFFSET].base_addr   = (unsigned int)vid_page_table >> MEM_OFFSET_BITS;
    vid_page_table[0].p = 1;    // present
    vid_page_table[0].r_w = 1;  // enable r/w
    vid_page_table[0].u_s = 1;  // user mode
    vid_page_table[0].base_addr = VID_PHYS_ADDR >> MEM_OFFSET_BITS;

    /* flush TLB */
    flush_TLB();

    /* success, return 0 */
    return 0;
}

//9 not implemented
int32_t set_handler()
{
    return -1;
}

//10 not implemented
int32_t sigreturn()
{
    return -1;
}

/* 
 *  vidmap
 *  Description: remaps user space virtual vidmem to a physical address
 *               (basically terminal vid buffer or physical vidmem)
 *  Input:  phys_addr -- a pointer points to the start of physical memory to map to
 *  Output: 0 for success, -1 for failure
 */
int32_t vid_remap(uint8_t* phys_addr)
{
    /* sanity check */
    if(phys_addr == NULL)
        return -1;

    /* remap video virtual memory */
    page_directory[VIDMAP_OFFSET].p           = 1;    // present
    page_directory[VIDMAP_OFFSET].r_w         = 1;    // enable r/w
    page_directory[VIDMAP_OFFSET].u_s         = 1;    // user mode
    page_directory[VIDMAP_OFFSET].base_addr   = (unsigned int)vid_page_table >> MEM_OFFSET_BITS;
    vid_page_table[0].p = 1;    // present
    vid_page_table[0].r_w = 1;  // enable r/w
    vid_page_table[0].u_s = 1;  // user mode
    vid_page_table[0].base_addr = ((uint32_t)phys_addr) >> MEM_OFFSET_BITS;

    /* flush TLB */
    flush_TLB();

    /* success, return 0 */
    return 0;
}

/*
 * get_new_pid
 * DESCRIPTION: get new process id by finding unoccupied position of pid_array
 * INPUT: none
 * OUTPUT: new process id
 * RETURN: new process id for success, -1 for fail
 * SIDE AFFECTS: pid_array corresponding to the pid would be set to busy
 */
uint32_t get_new_pid()
{
    int i;  /* loop index */
    /* traverse pid array to find unoccupied position */
    for (i = 0; i < NUM_PROCESS; i++)
    {
        if (!pid_array[i])
        {
            /* find a empty position, set entry to busy (1) */
            pid_array[i] = 1;
            return i;
        }
    }
    /* Current number of running process exceeds */
    printf("Current number of running process exceeds!\n");
    return -1;
}

/*
 * get_pcb_ptr
 * DESCRIPTION: get process's PCB pointer
 * INPUT: pid -- process id
 * OUTPUT: PCB of the process 
 * RETURN: PCB of the process 
 * SIDE AFFECTS: none
 */
inline pcb_t* get_pcb_ptr(uint32_t pid)
{
    return (pcb_t*)(KS_BASE_ADDR - KS_SIZE*(pid+1));
}

/*
 * file_op_table_init
 * DESCRIPTION: initialize file operation table array
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: file operation table initialized
 */
void file_op_table_init()
{
    /* init rtc operation table */
    file_op_table_arr[RTC_TYPE].open  = rtc_open;
    file_op_table_arr[RTC_TYPE].close = rtc_close;
    file_op_table_arr[RTC_TYPE].read  = rtc_read;
    file_op_table_arr[RTC_TYPE].write = rtc_write;

    /* init dir operation table */
    file_op_table_arr[DIR_TYPE].open  = dir_open ;
    file_op_table_arr[DIR_TYPE].close = dir_close;
    file_op_table_arr[DIR_TYPE].read  = dir_read ;
    file_op_table_arr[DIR_TYPE].write = dir_write;

    /* init file operation table */
    file_op_table_arr[FILE_TYPE].open  = file_open;
    file_op_table_arr[FILE_TYPE].close = file_close;
    file_op_table_arr[FILE_TYPE].read  = file_read;
    file_op_table_arr[FILE_TYPE].write = file_write;

    /* init stdin/out (terminal) operation table */
    file_op_table_arr[STD_TYPE].open  = terminal_open;
    file_op_table_arr[STD_TYPE].close = terminal_close;
    file_op_table_arr[STD_TYPE].read  = terminal_read;
    file_op_table_arr[STD_TYPE].write = terminal_write;
}
