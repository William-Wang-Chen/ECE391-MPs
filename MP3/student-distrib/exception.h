#ifndef _EXCEPTION_H
#define _EXCEPTION_H

/* exception number */
#define EXC_NUM     20

/* handler for exceptions in Linux */
extern void exc_divide_error();
extern void exc_single_step();
extern void exc_nmi();
extern void exc_breakpoint();
extern void exc_overflow();
extern void exc_bounds();
extern void exc_invalid_opcode();
extern void exc_coprocessor_not_avaliable();
extern void exc_double_fault();
extern void exc_coprocessor_segment_fault();
extern void exc_invalid_tss();
extern void exc_segment_not_present();
extern void exc_stack_fault();
extern void exc_general_protection_fault();
extern void exc_page_fault();
extern void exc_reserved();
extern void exc_math_fault();
extern void exc_alignment_check();
extern void exc_machine_check();
extern void exc_simd_floating_point();

#endif
