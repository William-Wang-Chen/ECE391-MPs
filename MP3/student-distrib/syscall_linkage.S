#define ASM     1
#include "syscall_linkage.h"

/* macro for push all genral registers and struct pt regs */
/* except eax */
#define pushall     \
    pushl   %fs;    \
    pushl   %es;    \
    pushl   %ds;    \
    pushl   %ebp;   \
    pushl   %edi;   \
    pushl   %esi;   \
    pushl   %edx;   \
    pushl   %ecx;   \
    pushl   %ebx;

/* macro for restore all genral registers and struct pt regs */
/* except eax */
#define popall      \
    popl    %ebx;   \
    popl    %ecx;   \
    popl    %edx;   \
    popl    %esi;   \
    popl    %edi;   \
    popl    %ebp;   \
    popl    %ds;    \
    popl    %es;    \
    popl    %fs;

/* system call linkage code */
.global system_call
system_call:
    /* save registers to stack */
    pushall
    /* chekc for a valid system call 1-10 */
    cmpl    $10, %eax
    jg      invalid_call
    cmpl    $1, %eax
    jl      invalid_call

    /* call specific routine using a jump table */
    call    *syscall_table(, %eax, 4)
    jmp     syscall_done

invalid_call:
    /* invalid system call number */
    movl    $-1, %eax

syscall_done:
    /* restore registers from stack */
    popall
    iret

/* jumptable for system calls */
syscall_table:
.long 0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
