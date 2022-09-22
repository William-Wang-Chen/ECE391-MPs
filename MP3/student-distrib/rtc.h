#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "syscall.h"

/* ports for RTC*/
#define RTC_PORT 0x70
#define RTC_DATA 0x71
/* default frequency*/
#define Default_Fre 2
/* register addresses for RTC*/
#define RTC_REGA 0x8A
#define RTC_REGB 0x8B
#define RTC_REGC 0x0C
/* frequency range*/
#define RTC_MIN_FRE 2
#define RTC_MAX_FRE 1024

/* freqency ratio array for processes, i.e. wait periods for virtualized rtc read */
int32_t virt_rtc_ratio[NUM_PROCESS];

/* initialize the rtc */
extern int32_t rtc_init();
/* set the frequency in RTC */
extern int32_t rtc_set_fre(int32_t fre);
/* the interrupt handler for rtc */
extern void rtc_handler();
/* open the rtc driver */
extern int32_t rtc_open(const char* filename);
/* RTC read. Virtualized. wait several periods cooresponding to current process' rtc frequency */
extern int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
/* RTC write.Virtualized. It reads the frequency in buf and set the corresponding process's RTC frequency. */
extern int32_t rtc_write(int32_t fd, void* buf, int32_t nbytes);
/*  close the RTC driver and reset some variable */
extern int32_t rtc_close(int32_t fd);

/* Old virtualized RTC read wrote in check point 2 */
// extern int32_t rtc_virtread(int32_t fd, void* buf, int32_t nbytes);

#endif // !_RTC_H
