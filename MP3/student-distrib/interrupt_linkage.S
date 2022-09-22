#define ASM     1
#include "interrupt_linkage.h"

/* macro for push all genral registers and struct pt regs */
#define pushall     \
    pushl   %fs;    \
    pushl   %es;    \
    pushl   %ds;    \
    pushl   %eax;   \
    pushl   %ebp;   \
    pushl   %edi;   \
    pushl   %esi;   \
    pushl   %edx;   \
    pushl   %ecx;   \
    pushl   %ebx;

/* macro for restore all genral registers and struct pt regs */
#define popall      \
    popl    %ebx;   \
    popl    %ecx;   \
    popl    %edx;   \
    popl    %esi;   \
    popl    %edi;   \
    popl    %ebp;   \
    popl    %eax;   \
    popl    %ds;    \
    popl    %es;    \
    popl    %fs;

/* RTC interrupt linkage code */
.global int_rtc
int_rtc:
    pushall
    cli
    call    rtc_handler
    sti
    popall
    iret

/* keyboard interrupt linkage code */
.global int_keyboard
int_keyboard:
    pushall
    cli
    call    keyboard_handler
    sti
    popall
    iret

/* PIT interrupt linkage code */
.global int_pit
int_pit:
    pushall
    cli
    call    pit_handler
    sti
    popall
    iret
