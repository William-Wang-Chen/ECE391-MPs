/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Reference: https://wiki.osdev.org/PIC & Lectures*/

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/*
 * i8259_init
 * DECRIPTION: Initialize the 8259 PIC. Notice It cannot implement the function of reset.
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: none
 */
void i8259_init(void) {
    /* mask out all interrupts on the PIC */
    outb(INIT_MASK, MASTER_8259_DATA);
    outb(INIT_MASK, SLAVE_8259_DATA);

    /* initialize the PIC with ICW1*/
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    /* initialize the PIC with ICW2*/
    outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);

    /* initialize the PIC with ICW3*/
    outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);

    /* initialize the PIC with ICW4*/
    outb(ICW4, MASTER_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA);

    // Mask all interrupts on the PIC. Thus any device needs to enable irq first.
    outb(INIT_MASK, MASTER_8259_DATA);
    outb(INIT_MASK, SLAVE_8259_DATA);
}

/*
 * enable_irq
 * DESCRIPTION: Enable (unmask) the specified IRQ. Notice enable the slave
 *              IRQ will not enable the IRQ2 (the port in master PIC for slave
 *              PIC), you need to enable the IRQ2 when using the slave PIC.
 * INPUT: irq_num: the number of irq
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: modify the status of PICs
 */
void enable_irq(uint32_t irq_num) {
    /* range judge. If it is out of range, immediately return*/
    if (irq_num > MAX_IRQ_NUM || irq_num <0)
        return;
    /* check whether it is master IRQ_num, i.e. 0-7*/
    if (irq_num < MAX_MASTER_IRQ_NUM)
    {
        /* it is master IRQ. Set the corresponding bit to 0 */
        master_mask = inb(MASTER_8259_DATA); // get the current status
        master_mask &= ~(1 << irq_num); // set corresponding bit to 0
        outb(master_mask, MASTER_8259_DATA); // push the new status
        return;
    }
    /* it is slave IRQ_num, i.e. 8-15. it is slave IRQ. Set the corresponding bit to 0 */
    slave_mask = inb(SLAVE_8259_DATA); // get the current status
    slave_mask &= ~(1 << (irq_num - (MAX_MASTER_IRQ_NUM))); // set corresponding bit to 0
    outb(slave_mask, SLAVE_8259_DATA); // push the new status
    return;
}

/*
 * disable_irq
 * DESCRIPTION: disable (mask) the specified IRQ. 
 * INPUT: irq_num: the number of irq
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: modify the status of PICs
 */
void disable_irq(uint32_t irq_num) {
    /* range judge. If it is out of range, immediately return*/
    if (irq_num > MAX_IRQ_NUM || irq_num < 0)
        return;
    /* check whether it is master IRQ_num, i.e. 0-7*/
    if (irq_num < MAX_MASTER_IRQ_NUM)
    {
        /* it is master IRQ. Set the corresponding bit to 1 */
        master_mask = inb(MASTER_8259_DATA); // get the current status
        master_mask |= 1 << irq_num; // set the corresponding bit to 1
        outb(master_mask, MASTER_8259_DATA); // push the new status
        return;
    }
    /* it is slave IRQ_num, i.e. 8-15*/
    slave_mask = inb(SLAVE_8259_DATA); // get the current status
    slave_mask |= 1 << (irq_num - MAX_MASTER_IRQ_NUM); // set the corresponding bit to 1
    outb(slave_mask, SLAVE_8259_DATA); // push the new status
    return;
}

/*
 * send_eoi
 * DESCRIPTION: send eoi when any interrupt handler finishes the work.
 * INPUT: irq_num: the number of irq
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: modify the status of PICs
 */
void send_eoi(uint32_t irq_num) {
    /* range judge. If it is out of range, immediately return*/
    if (irq_num > MAX_IRQ_NUM || irq_num < 0)
        return;
    /* check whether it is master IRQ_num, i.e. 0-7*/
    if (irq_num < MAX_MASTER_IRQ_NUM)
    {
        /* it is master PIC. Send the EOI | irq_num to master port*/
        outb(EOI | irq_num, MASTER_8259_PORT);
        return;
    }
    /* it is slave IRQ_num. Send EOI to both master port IRQ2 and slave port (irq_num-8)*/
    outb(EOI | (irq_num - MAX_MASTER_IRQ_NUM), SLAVE_8259_PORT);
    outb(EOI | SLAVE_PORT, MASTER_8259_PORT);
}
