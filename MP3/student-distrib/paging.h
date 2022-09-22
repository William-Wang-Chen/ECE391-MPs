/*
    paging.h header file.
*/

#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"

/* the number of paging directory entries */
#define NUM_PD_ENTRY        1024
/* the number of paging table entries */
#define NUM_PT_ENTRY        1024
/* the size of different types of page (number of bytes) */
#define PAGE_4KB_SIZE       4096
#define PAGE_4MB_SIZE       0x400000
/* 12bits offset in virtual memory as index in a 4kB-page */
#define MEM_OFFSET_BITS     12
#define ADDR_128MB          0x08000000
#define ADDR_132MB          0x08400000
#define ADDR_140MB          0x08c00000  /* 140MB */
#define VID_PHYS_ADDR       0xB8000
#define VID_VIRTUAL_ADDR    ADDR_140MB
#define VIDMAP_OFFSET       VID_VIRTUAL_ADDR/PAGE_4MB_SIZE          /* 140/4 */

/* struct for page directory entry */
typedef struct page_dir_entry
{
    uint32_t p              : 1;
    uint32_t r_w            : 1;
    uint32_t u_s            : 1;
    uint32_t pwt            : 1;
    uint32_t pcd            : 1;
    uint32_t a              : 1;
    uint32_t reserved       : 1;
    uint32_t ps             : 1;
    uint32_t g              : 1;
    uint32_t avail          : 3;
    uint32_t base_addr      : 20;
} page_dir_entry_t;

/* struct for page table entry */
typedef struct page_table_entry
{
    uint32_t p              : 1;
    uint32_t r_w            : 1;
    uint32_t u_s            : 1;
    uint32_t pwt            : 1;
    uint32_t pcd            : 1;
    uint32_t a              : 1;
    uint32_t d              : 1;
    uint32_t pat            : 1;
    uint32_t g              : 1;
    uint32_t avail          : 3;
    uint32_t base_addr      : 20;
} page_table_entry_t;

/* page directory, 4096 aligned */
page_dir_entry_t page_directory[NUM_PD_ENTRY] __attribute__((aligned(PAGE_4KB_SIZE)));
/* page table, 4096 aligned */
page_table_entry_t page_table[NUM_PT_ENTRY] __attribute__((aligned(PAGE_4KB_SIZE)));
/* page table for virtual video memory */
page_table_entry_t vid_page_table[NUM_PT_ENTRY] __attribute__((aligned(PAGE_4KB_SIZE)));

/* init paging */
void paging_init();
/* init page directory */
void page_directory_init();
/* init page table */
void page_table_init(); 
/* set hardware registers to enable mixed paging */
void enable_paging();
/* activate video memory page to be valid */
void activate_video();
/* set a page for according process */
void set_paging(uint32_t pid);
/* flush TLB */
void flush_TLB();

#endif
