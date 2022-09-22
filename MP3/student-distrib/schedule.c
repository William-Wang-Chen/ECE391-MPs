#include "schedule.h"
#include "terminal.h"
#include "syscall.h"
#include "x86_desc.h"
#include "lib.h"

/* Reference: https://wiki.osdev.org/Programmable_Interval_Timer */

/*
 * pit_init
 * DESCRIPTION: initialize the PIT, see schedule.h file for command details
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: none
 */
void pit_init()
{
    /* sent command to pit */
    outb(PIT_CMD, PIT_CMD_PORT);
    /* sent least significant bits of period */
    outb(PIT_LATCH && PIT_BITMASK, PIT_CHANNEL_0);
    /* sent most significant bits of period */
    outb(PIT_LATCH >> PIT_MSB_OFFSET, PIT_CHANNEL_0);
    /* enable interrupt */
    enable_irq(PIT_IRQ);
    return;
}

/*
 * pit_handler
 * DESCRIPTION: PIT handler, call scheduler to do scheduling
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: none
 */
void pit_handler()
{
    /* 
     * this send eoi CANNOT be put after scheduler because when the new executed 
     * shell (excute by terminal switch) comes back to terminal switch 
     * using esp ebp stored in execute, eoi would not be sent, so even if IF is 1, all interrupt would
     * fail because PIT has the highest priority.
     */
    send_eoi(PIT_IRQ);
    /* call scheduler */
    scheduler();
}

/*
 * scheduler
 * DESCRIPTION: do scheduling, switch between current running processes in different terminals
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: none
 */
void scheduler()
{
    int i;                          /* loop index for finding new process' terminal */
    pcb_t* curr_pcb;                /* current running process' pcb                 */
    pcb_t* next_pcb;                /* next process' pcb                            */
    uint32_t curr_process_term_id;  /* current running process' terminal id         */
    uint32_t next_term_id;          /* next process' terminal id                    */
    uint32_t next_pid;              /* next process id                              */

    /* if curr_pid is -1, which means the first process has not executed, just return */
    /* this cannot be removed because if it is removed, scheduler would switch to an inexistent */
    /* place and mess every thing up when the first shell hasn't been executed                  */
    if(curr_pid == -1)
        return;

    /* get current process ralevent info */
    curr_pcb = get_pcb_ptr(curr_pid);
    curr_process_term_id = curr_pcb->term_id;
    next_term_id = (curr_process_term_id + 1)%TERMINAL_NUM;

    /* get next process's terminal's id */
    for(i = 0; i < TERMINAL_NUM && !terminals[next_term_id].is_running; i++)
        next_term_id = (next_term_id + 1)%TERMINAL_NUM;
    
    /* if process does not change, which means there is only one process running, just return */
    if(next_term_id == curr_process_term_id)
        return;

    /* get next process's id */
    next_pid = terminals[next_term_id].curr_pid;

    /* set paging */
    set_paging(next_pid);

    /* remap video memory */
    if(next_term_id == curr_term_id)
        vid_remap((uint8_t *)VIDEO);
    else
        vid_remap(terminals[next_term_id].vid_buf);

    /* get next process's pcb */
    next_pcb = get_pcb_ptr(next_pid);

    /* set current fd array */
    cur_fd_array = next_pcb->fd_array;

    /* set kernel stack pointer */
    tss.esp0 = KS_BASE_ADDR - KS_SIZE * next_pid - sizeof(int32_t);

    /* update current pid */
    curr_pid = next_pid;

    /* store current's esp, ebp */
    asm volatile("                                \n\
        movl %%ebp, %0                            \n\
        movl %%esp, %1                            \n\
        "
        : "=r"(curr_pcb->ebp), "=r"(curr_pcb->esp)
        :
    );

    /* get next process's esp, ebp */
    asm volatile("                                \n\
        movl %0, %%ebp                            \n\
        movl %1, %%esp                            \n\
        "
        :
        : "r"(next_pcb->ebp), "r"(next_pcb->esp)
    );
}

