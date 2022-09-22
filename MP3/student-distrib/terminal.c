/*
    terminal driver
    support the open, close, read and write operations of the terminal
*/

#include "terminal.h"
#include "keyboard.h"
#include "syscall.h"
#include "lib.h"

/* MACRO for the sake of briefness */
#define CHECK_FAIL_RETURN(value) \
    if (value == -1)             \
    {                            \
        return -1;               \
    }

/*
 * terminal_init
 * DESCRIPTION: initialize all terminals structures
 * INPUT: none
 * OUTPUT: 0
 * RETURN: 0 if success, 1 if fail
 * SIDE AFFECTS: vid buffers' pages setted
 */
int32_t terminal_init()
{
    int i;  /* loop index for different terminals               */
    int j;  /* loop index for terminal buffer and video buffer  */
    /* init every terminal structures */
    for (i = 0; i < TERMINAL_NUM; i++)
    {
        /* set basic attribute */
        terminals[i].id = i;
        terminals[i].is_running = 0;
        terminals[i].curr_pid = -1;
        terminals[i].pnum = 0;
        terminals[i].cursor_x = 0;
        terminals[i].cursor_y = 0;
        terminals[i].is_enter = 0;
        terminals[i].term_buf_offset = 0;
        terminals[i].vid_buf = (uint8_t *)(VIDEO+(i+1)*PAGE_4KB_SIZE);
        /* init page for video buffer */
        set_vid_buf_page(i);
        /* init terminal buffer */
        for (j = 0; j < MAX_TERMINAL_BUF_SIZE; j++)
            terminals[i].term_buf[j] = '\0';
        /* init video buffer */
        for (j = 0; j < VIDBUF_SIZE/2; j++)
        {
            *(uint8_t *)(terminals[i].vid_buf + (j << 1)) = ' ';
            *(uint8_t *)(terminals[i].vid_buf + (j << 1) + 1) = ATTRIB;
        }
    }
    /* init current running terminal number */
    running_term_num = 0;
    return 0;
}

/*
 * terminal_switch
 * DESCRIPTION: switch to terminal with term_id, if the terminal is running, just switch;
 *              if not, run a shell for this new terminal
 *              ATTENTION: this program must be called from a program who has disable the interrupt
 * INPUT: term_id -- terminal id
 * OUTPUT: none
 * RETURN: 0 if success, 1 if fail
 * SIDE AFFECTS: virtual video memory for user remapped
 */
int32_t terminal_switch(uint32_t term_id)
{
    /* if it is the current terminal, do nothing */
    if (curr_term_id == term_id)
        return 0;

    /* save terminal info */
    CHECK_FAIL_RETURN(terminal_save(curr_term_id));

    /* restore terminal info */
    CHECK_FAIL_RETURN(terminal_restore(term_id));

    /* check whether the terminal is runnning */
    if (terminals[curr_term_id].is_running)
    {
        /* remap the virtual video memory because terminal changes */
        uint32_t curr_process_term_id = get_pcb_ptr(curr_pid)->term_id;
        if (curr_process_term_id != curr_term_id)
        {
            /* if the current process is executed by current terminal, remap virtual vidmem to physical vidmem */
            CHECK_FAIL_RETURN(vid_remap((uint8_t *)VIDEO));
        }
        else
        {
            /* if not, remap virtual vidmem to current process's terminal's video buffer */
            CHECK_FAIL_RETURN(vid_remap(terminals[curr_process_term_id].vid_buf));
        }
    }
    else
    {
        /* if it is the new terminal, run shell for this terminal */
        /* switch current process to the shell belongs to new terminal regardless of scheduler */
        
        /* update new terminal info, remap vidmem */
        terminals[curr_term_id].is_running = 1;
        running_term_num++;
        CHECK_FAIL_RETURN(vid_remap((uint8_t *)VIDEO));

        /* execute new shell for this new terminal */
        execute((uint8_t *)"shell");
    }

    /* success, return 0 */
    return 0;
}


/*
 * terminal_save
 * DESCRIPTION: save terminal info
 * INPUT: term_id -- terminal id
 * OUTPUT: none
 * RETURN: 0 if success, 1 if fail
 * SIDE AFFECTS: terminal vidbuffer changed
 */
int32_t terminal_save(uint32_t term_id)
{
    /* sanity check */
    if (term_id >= TERMINAL_NUM)
        return -1;

    /* save current cursor position */
    terminals[term_id].cursor_x = get_screen_x();
    terminals[term_id].cursor_y = get_screen_y();

    /* save video memory */
    memcpy((uint8_t *)terminals[term_id].vid_buf, (uint8_t *)VIDEO, VIDBUF_SIZE);

    /* success, return 0 */
    return 0;
}

/*
 * terminal_restore
 * DESCRIPTION: restore terminal info
 * INPUT: term_id -- terminal id
 * OUTPUT: none
 * RETURN: 0 if success, 1 if fail
 * SIDE AFFECTS: physical vidmem changed
 */
int32_t terminal_restore(uint32_t term_id)
{
    /* sanity check */
    if (term_id >= TERMINAL_NUM)
        return -1;

    /* set current terminal id */
    curr_term_id = term_id;

    /* restore current cursor position */
    set_screen_xy(terminals[term_id].cursor_x, terminals[term_id].cursor_y);

    /* restore video memory */
    memcpy((uint8_t *)VIDEO, (uint8_t *)terminals[term_id].vid_buf, VIDBUF_SIZE);

    /* success, return 0 */
    return 0;
}


/*
 * launch_first_terminal
 * DESCRIPTION: launch the first terminal and shell
 * INPUT: none
 * OUTPUT: none
 * RETURN: never return, 1 if fail
 * SIDE AFFECTS: none
 */
int32_t launch_first_terminal(){
    /* get init terminal info */
    CHECK_FAIL_RETURN(terminal_restore(FIRST_TERMINAL_ID));

    /* update terminal info */
    running_term_num = 1;
    terminals[FIRST_TERMINAL_ID].is_running = 1;

    /* launch the first shell */
    CHECK_FAIL_RETURN(execute((uint8_t*)"shell"));

    /* never reach here */
    return -1;
}

/*
*	terminal_open
*	Description:    open a terminal
*	inputs:         filename, not used
*	outputs:	    nothing
*	effects:	    simply does nothing and returns 0 so far
*/
int32_t terminal_open(const char *filename)
{
    return 0;
}

/*
*	terminal_close
*	Description:    close a terminal
*	inputs:         file descriptor fd
*	outputs:	    nothing
*	effects:	    simply does nothing and returns 0 so far
*/
int32_t terminal_close(int32_t fd)
{
    return 0;
}

/*
 * terminal_read
 * Description:    read the terminal input and store to 
 *                 the CURRENT RUNNING PROCESS' terminal's buffer
 * inputs:         fd      -- file descriptor
 *                 buf     -- a buffer that holds the terminal input
 *                 nbytes  -- the number of bytes to read from keyboard buffer
 * returns:        the actual number of bytes that are read successfully
 * effects:        read the keyboard input
 */
int32_t terminal_read(int32_t fd, void *buf, int32_t nbytes)
{
    /* sanity check to see whether the read operation is valid */
    if (NULL == buf || 0 == nbytes)
        return -1;

    /* loop index*/
    int i = 0;
    /* a flag to show whether the a \n is met in keyboard read buffer */
    int is_over = 0;
    /* return value, the number of bytes read, init to 0 */
    int ret = 0;

    /* current running process' terminal id */
    int curr_process_term_id;
    volatile uint8_t* read_buffer;

    /* 
        an infinite loop to wait
        until the foreground terminal program take an enter and is scheduled
        then the loop breaks
    */
    while (1)
    {
        /* get current running process' terminal id */
        curr_process_term_id = get_pcb_ptr(curr_pid)->term_id;
        if (terminals[curr_process_term_id].is_enter == 1)
            break;
    }

    /* disable interrupt, avoid shcduling causing some page fault */
    cli();

    /* get current running process' terminal buffer */
    read_buffer = terminals[curr_process_term_id].term_buf;

    /* 
        iterate through the keyboard read buffer
        (i < 127) handles buffer overflow, at most 127 chars are legal to read
    */
    for (i = 0; (i < nbytes) && (i < MAX_TERMINAL_BUF_SIZE - 1); i++)
    {
        /* fill the buf with terminal input */
        ((char *)buf)[i] = read_buffer[i];

        /* if current char is enter, the input is over */
        if (read_buffer[i] == '\n')
        {
            /* set is_over and change the current, last char in buffer to be \0 */
            is_over = 1;
            ((char *)buf)[i] = '\0';
        }
        /* if the input hasn't over, increment return value */
        if (is_over == 0)
            ret++;
    }

    /* clear read buffer for current process' terminal */
    clr_read_buffer();

    /* enable interrupt */
    sti();

    return ret;
}

/*
 *  terminal_write
 *  Description:    write the corresponding number of bytes of a buffer of the terminal
 *                  if the current process' terminal is foreground terminal, write chars into video mem
 *                  if not, write to this terminal's video buffer
 *  inputs:         fd      -- file descriptor
 *                  buf     -- a buffer that holds the chars to write to terminal
 *                  nbytes  -- the number of bytes to write from the input buffer
 *  returns:        the actual number of bytes that are write successfully
 *  outputs:        the chars in the buffer are outputted to terminal
 *  effects:        write infomation to terminal' vidbuffer or vidmem
 */
int32_t terminal_write(int32_t fd, void *buf, int32_t nbytes)
{
    /* sanity check to see whether the write operation is valid */
    if (NULL == buf || 0 == nbytes)
        return -1;

    /* loop index */
    int i = 0;
    /* return value, the number of bytes written, init to 0 */
    int ret = 0;

    /* disable interrupt, avoid scheduling problem */
    cli();

    /* check whether current process' terminal is the foreground terminal */
    if (get_pcb_ptr(curr_pid)->term_id == curr_term_id)
    {
        /* iterate through the input buffer */
        for (i = 0; i < nbytes; i++)
        {
            if (((char *)buf)[i] != '\0')
            {
                /* we only write none \0 char to terminal */
                putc(((char *)buf)[i]);
                /* increment ret */
                ret++;
            }
        }
    }
    else
    {
        /* if it is not the foreground terminal, write into this terminal's video buffer */
        for (i = 0; i < nbytes; i++)
        {
            if (((char *)buf)[i] != '\0')
            {
                /* we only write none \0 char to terminal */
                terminal_putc(((char *)buf)[i]);
                /* increment ret */
                ret++;
            }
        }
    }

    /* enable interrupt */
    sti();

    /* return the number of bytes written */
    return ret;
}

/*
 * set_vid_buf_page
 * DESCRIPTION: set terminal's video buffer page according to the terminal id
 *              no need to set page directory because vid buffer's address is in 0-3MB, which has enabled
 * INPUT: none
 * OUTPUT: none
 * RETURN: never return, 1 if fail
 * SIDE AFFECTS: none
 */
void set_vid_buf_page(int term_id){

    /* get page table index */
    uint32_t index = (uint32_t)(terminals[term_id].vid_buf) >> MEM_OFFSET_BITS;

    /* set paging */
    page_table[index].p = 1;        // Present
    page_table[index].r_w = 1;      // Read/write permission, always 1
    page_table[index].u_s = 0;      // User/supervisor, 0 means supervisor mode
    page_table[index].pwt = 0;      // Page write-through, always 0
    page_table[index].pcd = 0;      // Page cache disabled
    page_table[index].a = 0;        // Accessed, won't use, does not matter
    page_table[index].d = 0;        // Dirty, set to 0
    page_table[index].pat = 0;      // Page Attribute Table index, set to 0
    page_table[index].g = 1;        // Global bit, 1 means page is global and would not be clear in TLB when flush TLB
    page_table[index].avail = 0;    // Available for our use, won't use, does not matter
    page_table[index].base_addr = index;// Page-Table Base Address

    /* flush TLB */
    flush_TLB();
}
