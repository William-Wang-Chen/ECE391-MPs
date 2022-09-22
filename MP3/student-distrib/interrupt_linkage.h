#ifndef _INTERRUPT_LINKAGE_H
#define _INTERRUPT_LINKAGE_H
#ifndef ASM

/* RTC interrupt linkage code */
extern void int_rtc();
/* keyboard interrupt linkage code */
extern void int_keyboard();
/* PIT interrupt linkage code */
extern void int_pit();

#endif
#endif
