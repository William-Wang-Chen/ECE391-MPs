#include "lib.h"
#include "x86_desc.h"
#include "idt.h"
#include "exception.h"
#include "interrupt_linkage.h"

// just for check point 3.1
void system_call();

/* 
 * idt_init
 *   DESCRIPTION: Initialize IDT (interrupt descriptor table)
 *                0x00-0x13 are exceptions
 *                0x21 keyboard interrupt
 *                0x28 RTC interrupt
 *                0x80 reserved for system call
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes IDT table
 */
void idt_init(){
    // Exception
    set_intr_gate(0x00, exc_divide_error);
    set_intr_gate(0x01, exc_single_step);
    set_intr_gate(0x02, exc_nmi);
    set_intr_gate(0x03, exc_breakpoint);
    set_intr_gate(0x04, exc_overflow);
    set_intr_gate(0x05, exc_bounds);
    set_intr_gate(0x06, exc_invalid_opcode);
    set_intr_gate(0x07, exc_coprocessor_not_avaliable);
    set_intr_gate(0x08, exc_double_fault);
    set_intr_gate(0x09, exc_coprocessor_segment_fault);
    set_intr_gate(0x0A, exc_invalid_tss);
    set_intr_gate(0x0B, exc_segment_not_present);
    set_intr_gate(0x0C, exc_stack_fault);
    set_intr_gate(0x0D, exc_general_protection_fault);
    set_intr_gate(0x0E, exc_page_fault);
    set_intr_gate(0x0F, exc_reserved);
    set_intr_gate(0x10, exc_math_fault);
    set_intr_gate(0x11, exc_alignment_check);
    set_intr_gate(0x12, exc_machine_check);
    set_intr_gate(0x13, exc_simd_floating_point);
    // Interrupt
    set_intr_gate(0x20, int_pit);
    set_intr_gate(0x21, int_keyboard);
    set_intr_gate(0x28, int_rtc);
    // System Call
    set_trap_gate(0x80, system_call);
    return;
}

/* 
 * set_intr_gate
 *   DESCRIPTION: Set interrupt gate in IDT (interrupt descripter table) of one interrupt
 *                using the address of the interrupt handler
 *                see x86_desc.h for formats
 *   INPUTS: vec -- corresponding interrupt vector
 *           addr -- address of interrupt handler
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes IDT table
 */
void set_intr_gate(unsigned int vec, void *addr){
    if(addr == NULL || vec >= NUM_VEC)
        return;
    SET_IDT_ENTRY(idt[vec], addr);
    idt[vec].seg_selector = KERNEL_CS;      // kernel segment code
    idt[vec].reserved4 = 0;                 // interrupt specified
    idt[vec].reserved3 = 1;                 // interrupt specified
    idt[vec].reserved2 = 1;                 // interrupt specified
    idt[vec].reserved1 = 1;                 // interrupt specified
    idt[vec].size = 1;                      // indicate 32 bit interrupt gate
    idt[vec].reserved0 = 0;                 // interrupt specified
    idt[vec].dpl = KERNEL_PRIORITY;         // kernel priority (0)
    idt[vec].present = 1;                   // entry present
    return;
}

/* 
 * set_trap_gate
 *   DESCRIPTION: Set trap gate in IDT (interrupt descripter table) of system call
 *                using the address of system call linkage code
 *                see x86_desc.h for formats
 *   INPUTS: vec -- trap vector (0x80 for system call)
 *           addr -- address of the system call linkage code
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes IDT table
 */
void set_trap_gate(unsigned int vec, void *addr){
    if(addr == NULL || vec >= NUM_VEC)
        return;
    SET_IDT_ENTRY(idt[vec], addr);
    idt[vec].seg_selector = KERNEL_CS;      // kernel segment code
    idt[vec].reserved4 = 0;                 // trap specified
    idt[vec].reserved3 = 1;                 // trap specified
    idt[vec].reserved2 = 1;                 // trap specified
    idt[vec].reserved1 = 1;                 // trap specified
    idt[vec].size = 1;                      // indicate 32 bit trap gate
    idt[vec].reserved0 = 0;                 // trap specified
    idt[vec].dpl = USER_PRIORITY;           // user priority for system call (3)
    idt[vec].present = 1;                   // entry present
    return;
}

// /* 
//  * old system call handler
//  *   DESCRIPTION: temp handler for system call
//  *   INPUTS: none
//  *   OUTPUTS: none
//  *   RETURN VALUE: none
//  *   SIDE EFFECTS: none
//  */
// void system_call(){
//     printf("a system call was called.");
// }
