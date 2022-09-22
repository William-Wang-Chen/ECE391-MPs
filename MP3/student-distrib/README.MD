# BUG-LOG && DEV-LOG

#### 2021/03/20 Guan

- Checkpoint 1 GDT & IDT init done
- seems no bug (currently)

#### 2021/03/20 Xie

+ Checkpoint 1 paging init done, all tests passes.
+ a werid bug in ./paging.c: void enable_paging();

        x86-asm
        // IF WRITE IN-LINE ASM IN THE FOLLOWING WAY,
        // THE PROGRAM WILL CRASH
        asm volatile(
                // Enable Mixture of 4kb and 4mb access
                "movl %cr4, %eax;"
                "orl $0x00000010, %eax;"
                "movl %eax, %cr4;"
        
                // MSE: enable paging; LSE: enable protection mode
                "movl %cr0, %eax;"
                "orl $0x80000000, %eax;"
                "movl %eax, %cr0;"
        
                //  load cr3 base addr
                "movl $page_directory, %eax;"
                "andl $0xFFFFFC00, %eax;"
                "movl %eax, %cr3;"
        );
        
        // BUT IN THIS WAY IT WORKS
        // TO BE MORE SPECIFIC, ANY OTHER ORDER CRASHES
        asm volatile(
                // load cr3 base addr 
                "movl $page_directory, %eax;"
                "andl $0xFFFFFC00, %eax;"
                "movl %eax, %cr3;"
        
                // Enable Mixture of 4kb and 4mb access
                "movl %cr4, %eax;"
                "orl $0x00000010, %eax;"
                "movl %eax, %cr4;"
        
                // MSE: enable paging
                "movl %cr0, %eax;"
                "orl $0x80000000, %eax;"
                "movl %eax, %cr0;"
        );
+ What remains to do: a better interface and wrapper of my test code.

#### 2021/03/20 YY
+ PIC initialization and interface finished. No large bug found.

#### 2021/03/21 Li
+ Checkpoint 1 keyboard initialization done.
+ a bug in ./keyboard.h: #define KEYBOARD_PORT 0x21
        should use 0x60 instead of 0x21, since it's keyboard's port index,
        and 0x21 causes error because it's master PIC port.

#### 2021/03/21 YY
+ rtc initialization and primary handler finished.
+ a bug for rtc handler: after each interrupt, the content in register C 
    should be thrown away by reading the register. 

#### 2021/03/21 Guan

- Exception test finished.

#### 2021/03/22 Guan

- Garbage input test finished.

#### 2021/03/27 Guan

- file system finished, not tested. One thing needs to be noticed is that, index starts from 0, so when we compare index with some max_num, we need to use >= or <=.

#### 2021/03/28 Li

+ Function keys added for keyboard.
+ Implemented read buffer.


#### 2021/03/28 Xie
+ terminal_open, terminal_close, terminal_read and terminal_write finished and debugged but not tested.
+ Modify keyboard driver slightly


#### 2021/03/29 Li

+ Implemented screen scrolling.
+ Fixed a bug that caused backspace to overwrite previous rows.
+ RTC test for rtc_read() and rtc_write finished.

#### 2021/03/29 Xie
+ terminal driver test function done.
+ terminal driver test finished.
+ a worthnoting bug, at first there is not a way to sync between keyboard and the terminal. So when performing terminal_read, nothing can be read and the screen would flash with prompt information of test function. As a solution, I introduce a flag called is_ready to communicate between keyboard and terminal. Only when the enter is pressed would this flag be set, telling the terminal a new read is ready.

#### 2021/03/29 Guan

- file system tested over. Surprisingly, no significant bug. One of the insignificant bugs is, I write an always false sanity check in read data. Other bugs are just syntax error or warning.

#### 2021/04/4 Guan

- open/close/read/write finished
- system call assembly code finished
- system call frame work initialized
- no debug

#### 2021/04/4 Xie

- command parser finished
- paging setting finished

- no debug

#### 2021/04/11 YY

- execute & halt finished
- no debug

#### 2021/04/12 Guan

- execute & halt modified
- debug finished
- **BUG1:** page fault once we start the machine
  - reason: first instruction address is wrong because we uses the address of the entry point not the content
  - solution: use the content as eip
- **BUG2:** general protection fault
  - reason: Midterm 2: did not set DPL of trap gate
  - solution: set DPL of trap gate to 3
- **BUG3:** cannot open RTC
  - reason: return 0 but not fd in open
  - solution: return fd in open
- **BUG4:** std_in_out test failed
  - reason: not forbid stdin/stdout from write/read
  - solution: add sanity check for write and read
- **BUG5:** error_unopened test failed when tested together; success when tested individually.
  - reason:  sanity check, close would clear the fd if device close return 0; however we let close dir return -1, thus the fd array of dir would not be cleared even if it is closed.
  - solution: let close dir return 0

#### 2021/04/18 Li


- getargs & vidmap finished
- debug finished
- **BUG1:** page fault when executing fish
  - reason: used wrong address for 140MB.
  - solution: change ADDR_140MB from 0x4600000 to 0x8c00000.

#### 2021/04/19 Guan

- Bug fixed:
- **BUG1**: when enter rtc and ., the system would execute last program
  - reason: did not check file type when do executable check 
  - solution: check file type

- **BUG2**: grep print out something when we does not enter arg
  - reason: did not check no argument case in get_args
  - solution: check no argument case in get_args

#### 2021/04/24 Guan

- Small Bug fixed:
- 1. Add a break line when halt the program to deal with the deficiency of shell program: it does not change to a new line when a program is terminated.
  2. Add a sanity check for a filename with length bigger than 32 in find_dentry_by_name().

#### 2021/04/27 Xie

- terminal modified

#### 2021/04/27 Guan

- scheduler & terminal finished (original code haven't change)

- undebugged

#### 2021/04/28 Guan

- syscall modified for scheduling
- undebugged

#### 2021/04/28 Li

- Alt + F keys for terminals finished
- putc won't print NULL keys
- undebugged

#### 2021/04/29 Guan

- **BUG1:**  **PAGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART**
  - reason: Enable interrupt when initializing RTC, which allows scheduling before initializing terminals. 
  - solution: set curr_pid to -1 initially, and add a conditional check for this at the begin of the scheduler 

#### 2021/5/1&2 Guan

- **BUG1**: **PAGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART & AGE FALUT & RESTART**
- This bug is **extremely important**, it is because when we execute a new terminal and a new shell, the scheduler would try to restore esp and ebp to back to first shell, however, we did not store the esp & ebp before. Then I thought about it for a while, because we need to keep a similar stack status as scheduler when we store the esp & ebp before executing. Finally I figured it out by store esp & ebp at the end of execution.
- when a new shell in a new terminal is executed, a schedule interrupt would happen and try to restore the stack info of the current program. However, the new shell is not entered by scheduler() but the execute() function. Therefore, the scheduler would let the system restore the execute() stack status and would continue executing scheduler()'s instruction, which is leave & ret. So the system would back to terminal_switch() from execute(), then return to keyboard function finally return to interrupt linkage code, then enable interrupt and return from interrupt linkage code.
- **BUG2:** When switch to another terminal, interrupt is disable.
  - reason: I put the send_eoi() after the scheduler() in PIT handler. This send eoi CANNOT be put after scheduler because when the new executed shell (excute by terminal switch) comes back to terminal switch (because of the scheduler) using esp ebp stored in execute, eoi would not be sent, so even if IF is 1, all interrupt would fail because PIT IRQ has the highest priority.
  - solution: put send eoi before scheduler()
- **BUG3:** When a program halt and the foreground terminal is not its terminal, we cannot execute anything in the foreground terminal
  - reason: wrongly use the foreground terminal but not the current running process' terminal in the halt()
  - solution: change it to the current running process' terminal. 
- **BUG4:** When a program halt, the system would crash and restart sometimes (frequently)
  - reason:  in halt(), we use sti() too early, so the scheduler interrupt halt and causes some problem
  - solution: put sti in the assembly code just before iret
- **BUG5:** After fish halt, the frequency of pingpong become extremely low
  - reason: fish calls rtc_close(), who reset the rtc frequency to the default frequency
  - solution: virtualize the RTC read and write