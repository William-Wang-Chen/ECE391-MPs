#include "rtc.h"
#include "types.h"
#include "lib.h"
#include "i8259.h"
#include "tests.h"
#include "terminal.h"

/* Reference: https://wiki.osdev.org/RTC */

// static int32_t virt_counter; // used to serve as counter in rtc_virtread, not used now

/* count rtc interrupt, would overflow, but doesn't matter */
/* used for indicate whether a new interrupt happen or for virtualization */
static uint32_t rtc_counter;

/*
 * rtc_init
 * DESCRIPTION: initialize the rtc.
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: none
 */
int32_t rtc_init()
{
    int i;      /* loop index to init virtual rtc ratio array */
    uint8_t prev;

    /* disable NMI*/
    prev = inb(RTC_PORT) | 0x80; //0x80 is used to set the first bit to 1
    outb(prev, RTC_PORT);

    /* set default frequency. if fails, return -1*/
    if (rtc_set_fre(Default_Fre))
        return -1;

    /* set the PIE bit in register B to 1. Thus we can enable the interrupt of RTC*/
    outb(RTC_REGB, RTC_PORT); // set port index
    prev = inb(RTC_DATA); // get original status
    outb(RTC_REGB, RTC_PORT); // set again
    outb(prev | 0x40, RTC_DATA); // set corresponding bit

    /* enable corresponding IRQ for PICs*/
    enable_irq(RTC_IRQ);
    enable_irq(SLAVE_IRQ);

    /* initialize the global variable */
    // virt_counter = 0;
    rtc_counter = 0;

    /* init virtual rtc ratio array */
    for(i = 0; i < NUM_PROCESS; i++)
        virt_rtc_ratio[i] = RTC_MAX_FRE/RTC_MAX_FRE;

    /* enable NMI*/
    prev = inb(RTC_PORT) & 0X7F; //0x7F is used to set the first bit to 0
    outb(prev, RTC_PORT);

    return 0;
}

/*
 * rtc_set_fre
 * DESCRIPTION: set the frequency in RTC
 * INPUT: fre: the target frequency
 * OUTPUT: none
 * RETURN: none
 * SIDE AFFECTS: none
 */
int32_t rtc_set_fre(int32_t fre)
{
    int log = 0;
    uint8_t rate;
    uint8_t prev;
    /* check whether the frequency is in valid range*/
    if (fre < RTC_MIN_FRE || fre > RTC_MAX_FRE)
        return -1;
    /* check whether the frequency is in power of 2*/
    if (((fre - 1) & fre))
        return -1;
    /* get the log2 value of frequency*/
    while (fre >>= 1)
        log += 1;
    /* get the corresponding rate due to the table 3*/
    rate = 16 - log; //16 is the number of bit patterns in Register A0,A1,A2,A3 due to table 3

    /* disable NMI*/
    prev = inb(RTC_PORT) | 0x80; //0x80 is used to set the first bit to 1
    outb(prev, RTC_PORT);

    /* set the frequency bits in register A to the rate value*/
    outb(RTC_REGA, RTC_PORT); // set index to register A
    prev = inb(RTC_DATA); // get value
    outb(RTC_REGA, RTC_PORT); // set index again
    outb((prev & 0xF0) | rate, RTC_DATA); // set the corresponding bits (0-3) 

    /* enable the NMI*/
    prev = inb(RTC_PORT) & 0X7F; //0x7F is used to set the first bit to 0
    outb(prev, RTC_PORT);

    /* success, return 0 */
    return 0;
}

/*
 * rtc_handler
 * DESCRIPTION: the interrupt handler for rtc
 * INPUT: none
 * OUTPUT: none
 * RETURN: none
 * SIDEAFFECTS: none
 */
void rtc_handler() {
    /* test RTC*/
    if(TEST_RTC)
        test_interrupts();

    outb(RTC_REGC, RTC_PORT); // select register C
    inb(RTC_DATA); // throw the contents in register C to reset status bits in register C

    rtc_counter++; // update counter

    /* send EOI to indicate the handler finishes the work*/
    send_eoi(RTC_IRQ);
}

/*
 * rtc_open
 * DESCRIPTION: this function serves as the open call for RTC driver
 * INPUT: filename: unused variable
 * OUTPUT: none
 * RETURN: return 0 for success
 * SIDEAFFECTS: none 
 */
int32_t rtc_open(const char* filename)
{
    /* set default frequency*/
    rtc_set_fre(RTC_MAX_FRE);
    /* return 0 for success*/
    return 0;
}

/*
 * rtc_close
 * DESCRIPTION: this function serves as the close call for RTC driver and reset some variable
 * INPUT: fd: unused variable
 * OUTPUT: none
 * RETURN: return 0 for success
 * SIDEAFFECTS: none
 */
int32_t rtc_close(int32_t fd)
{
    /* initialize the virtread counter to 0*/
    // virt_counter = 0;

    /* set default frequency and clear virtual rtc ratio array */
    rtc_set_fre(RTC_MAX_FRE);
    virt_rtc_ratio[curr_pid] = RTC_MAX_FRE/RTC_MAX_FRE;

    /* return 0 for success*/
    return 0;
}

/*
 * rtc_read
 * DESCRIPTION: a virtualized rtc read, wait several periods cooresponding to current process' rtc frequency
 * INPUT: fd, buf, nbytes: unused variable
 * OUTPUT: none
 * RETURN: return 0 for success
 * SIDEAFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes)
{
    /* record the current rtc_interrupt_status */
    int32_t current_status = rtc_counter;
    /* calculate wait period, because there is scheduling, divide it by the number of running terminals */
    int32_t wait_period = virt_rtc_ratio[curr_pid] / running_term_num;

    /* if wait period is too short, set it to 1 */
    if(wait_period == 0)
        wait_period = 1;

    /* if next rtc interrupt doesn't come, wait */
    while(current_status == rtc_counter);
    /* wait for several period */
    while(rtc_counter % wait_period);

    /* return 0 for success*/
    return 0;
}

/*
 * rtc_write
 * DESCRIPTION: a virtualized rtc write. It reads the frequency in buf 
 *              and set the corresponding process's RTC frequency. It only receive int32_t as valid input.
 *              freqency who is not satisfied would be set to the nearest virtual freqency
 * INPUT: fd: unused variable
 *        buf: array used to store the target frequency.
 *        nbytes: the number of byte in buf
 * OUTPUT: none
 * RETURN: return 0 for success
 * SIDEAFFECTS: current process' rtc freqency changed
 */
int32_t rtc_write(int32_t fd, void* buf, int32_t nbytes)
{
    int32_t virt_freq;  /* virtual freqency */

    /* check whether the byte numbers is 4. If not, immediately return */
    if (nbytes != 4)
        return -1;

    /* check whether the buff is NULL */
    if ((int32_t*)buf == NULL)
        return -1;

    /* get the freqency */
    virt_freq = *(int32_t*)buf;

    /* sanity check */
    if (virt_freq > RTC_MAX_FRE || virt_freq <= 0)
        return -1;

    /* set freqency ratio, i.e. wait periods for virtualized rtc read */
    /* e.g. if we want 512 Hz freqency, wait every 1024/512 = 2 interrupt period */
    virt_rtc_ratio[curr_pid] = RTC_MAX_FRE / virt_freq;

    /* success, return 0 */
    return 0;
}

/* old virtualized rtc read, wrote in Check Point 2 */
/*
 * rtc_virtread
 * DESCRIPTION: this function implements the virtualization RTC. Notice this function receives proportion as input
 *              and will change the RTC frequency to the max frequency. It only receive int32_t as valid input.
 * INPUT: fd: unused variable
 *        buf: array used to store the target proportion
 *        nbytes: the number of byte in buf  
 * OUTPUT: none
 * RETURN: 0 for success
 * SIDEAFFECTS: set RTC frequency to max frequency.
 */
// int32_t rtc_virtread(int32_t fd, void* buf, int32_t nbytes)
// {
//     // check whether the byte numbers is 4. If not, immediately return
//     if (nbytes != 4)
//         return -1;
//     // check whether the buff is NULL
//     if ((int32_t*)buf == NULL)
//         return -1;
//     /* change to max frequency */
//     rtc_set_fre(RTC_MAX_FRE);
//     /* get the target proportion */
//     int32_t threshold = *(int32_t*)buf;
//     /* wait until proportion number of rtc_read called*/
//     while (virt_counter < threshold)
//     {
//         rtc_read(fd, buf, nbytes); // read once
//         virt_counter++; // increase the counter
//     }
//     /* reset the counter */
//     virt_counter = 0;
//     /* return 0 for success*/
//     return 0;
// }
