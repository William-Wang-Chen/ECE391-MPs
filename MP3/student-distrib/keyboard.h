#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define KEY_NUM             60
#define KEYBOARD_PORT       0x60
#define KEYBOARD_IRQ        1
#define READ_BUFFER_SIZE    127
#define BACKSPACE	        0x0E
#define TAB			        0x0F
#define ENTER		        0x1C
#define CAPS_LOCK	        0x3A
#define LSHIFT_DOWN	        0x2A
#define LSHIFT_UP	        0xAA
#define RSHIFT_DOWN	        0x36
#define RSHIFT_UP	        0xB6
#define CTRL_DOWN	        0x1D
#define CTRL_UP		        0x9D
#define ALT_DOWN	        0x38
#define ALT_UP		        0xB8
#define F1          		0x3B
#define F2          		0x3C
#define F3          		0x3D

/* init the keyboard by enabling the corresponding irq line */
extern void keyboard_init();
/* keyboard interrupt handler */
extern void keyboard_handler();
/* echo a pressed key to screen */
extern void print_key(unsigned char scancode);
/* clear the keyboard read buffer */
extern void clr_read_buffer();


#endif
