/*
    terminal driver header file
*/

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

#define MAX_TERMINAL_BUF_SIZE   128
#define TERMINAL_NUM            3
#define FIRST_TERMINAL_ID       0

/* terminal info struct */
typedef struct terminal_t{

    uint32_t id;            /* terminal id                                  */
    uint32_t is_running;    /* indicate whether the terminal is running     */
    uint32_t curr_pid;      /* current process id of THIS terminal          */
    uint32_t pnum;          /* number of process running in this terminal   */
    uint32_t cursor_x;      /* cursor x position of this terminal           */
    uint32_t cursor_y;      /* cursor y position of this terminal           */
    volatile uint32_t is_enter;                         /* indicate whether enter is pressed for this terminal */
    volatile uint8_t term_buf[MAX_TERMINAL_BUF_SIZE];   /* read buffer for this terminal                       */
    volatile uint8_t term_buf_offset;                   /* offset of read buffer for this terminal             */
    uint8_t *vid_buf;                                   /* pointer points to this terminal's video buffer      */

} terminal_t;

/* array of terminal info */
terminal_t terminals[TERMINAL_NUM];

/* foreground terminal id */
uint32_t curr_term_id;

/* current running terminal number */
uint32_t running_term_num;

/* initialize all terminals structures */
int32_t terminal_init();

/* switch to terminal with term_id */
int32_t terminal_switch(uint32_t term_id);

/* save terminal info */
int32_t terminal_save(uint32_t term_id);

/* restore terminal info */
int32_t terminal_restore();

/* launch the first terminal and shell */
int32_t launch_first_terminal();

/* open a terminal */
int32_t terminal_open(const char* filename);

/* close a terminal */
int32_t terminal_close(int32_t fd);

/* read the terminal input and store to a buffer */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

/* write the corresponding number of bytes of a buffer to the terminal */
int32_t terminal_write(int32_t fd, void* buf, int32_t nbytes);

/* set terminal's video buffer page according to the terminal id */
void set_vid_buf_page(int i);

#endif
