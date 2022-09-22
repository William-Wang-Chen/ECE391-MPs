/*
    paging.c
*/

#include "paging.h"
#include "lib.h"

/*
*	paging_init
*	Description:    init paging when booting system
*	inputs:         nothing
*	outputs:        nothing
*	effects:        the paging mechanism is enabled and initialized 
*/
void paging_init()
{
    /* init empty page directory and a page table */
    page_directory_init();
    page_table_init();

    /* activate 0-3mB memory with 4kB pages, connect to the page table */
    page_directory[0].p = 1;
    page_directory[0].base_addr = (unsigned int)page_table >> MEM_OFFSET_BITS;

    /* activate 4-7mB memory with a 4mB page */
    page_directory[1].p = 1;
    page_directory[1].ps = 1;
    page_directory[1].g = 1;
    /* 
        for 4mB page, set bit 13-21 to be 0, and the 10 bit msb should be 1
        so 00000000 01 00 0000 0000 = 0x00400, we should set it to be 0x400
    */
    page_directory[1].base_addr = 0x400;

    /* manipulate hardware, enable paging */
    enable_paging();
    /* activate the video memory page */
    activate_video();
}

/*
*	page_directory_init
*	Description:    init a page directory with each entry's presence to be 0
*	inputs:		    nothing
*	outputs:	    nothing
*	effects:	    a page directory is initialized for later usage, all its entries are r/w
*/
void page_directory_init()
{
    /* loop index */
    int i;
    /* interate the directory to fill each entry */
    for (i = 0; i < NUM_PD_ENTRY; i++)
    {
        page_directory[i].p = 0;        // Present
        page_directory[i].r_w = 1;      // Read/write permission always 1
        page_directory[i].u_s = 0;      // User/supervisor
        page_directory[i].pwt = 0;      // Page write-through always 0
        page_directory[i].pcd = 0;      // Page cache disabled
        page_directory[i].a = 0;        // Accessed, won't use, does not matter
        page_directory[i].reserved = 0; // Reserved, set to 0
        page_directory[i].ps = 0;       // Page size, 0 for 4kB; 1 for 4mB
        page_directory[i].g = 0;        // Global bit
        page_directory[i].avail = 0;    // Available for our use, won't use, does not matter
        page_directory[i].base_addr = 0;// Page-Table Base Address
    }
}

/*
*	page_table_init
*	Description:    init a page table with each entry's presence to be 0
*	inputs:		    nothing
*	outputs:	    nothing
*	effects:	    a page table is initialized for later usage, all its entries are r/w
*/
void page_table_init()
{
    /* loop index */
    int i;
    /* interate the table to fill each entry */
    for (i = 0; i < NUM_PT_ENTRY; i++)
    {
        page_table[i].p = 0;        // Present
        page_table[i].r_w = 1;      // Read/write permission, always 1
        page_table[i].u_s = 0;      // User/supervisor
        page_table[i].pwt = 0;      // Page write-through, always 0
        page_table[i].pcd = 0;      // Page cache disabled
        page_table[i].a = 0;        // Accessed, won't use, does not matter
        page_table[i].d = 0;        // Dirty, set to 0
        page_table[i].pat = 0;      // Page Attribute Table index, set to 0
        page_table[i].g = 0;        // Global bit
        page_table[i].avail = 0;    // Available for our use, won't use, does not matter
        page_table[i].base_addr = i;// Page-Table Base Address
    }
}

/*
*	enable_paging
*	Description:    several hardware registers' values are set to enable paging 
*	inputs:		    nothing
*	outputs:	    nothing
*	effects:	    mixed 4kB/4mB paging is enabled and cr3 base addr is set 
*/
void enable_paging()
{   
    asm volatile(
        /* load cr3 base addr */
        "movl $page_directory, %eax;"
        /* mask unnecessary bits, the last 10 bits would come from virtual memory addr as index */
        "andl $0xFFFFFC00, %eax;"   
        "movl %eax, %cr3;"

        /* Enable Mixture of 4kb and 4mb access */
        "movl %cr4, %eax;"
        /* set the bit 4 to be 1 */
        "orl $0x00000010, %eax;"
        "movl %eax, %cr4;"

        /* MSE: enable paging */
        "movl %cr0, %eax;"
        /* set the bit 31 to be 1 */
        "orl $0x80000000, %eax;"
        "movl %eax, %cr0;"
    );
}

/*
*	activate_video
*	Description:    activate a page for video memory usage
*	inputs:		    nothing
*	outputs:	    nothing
*	effects:	    video memory is now valid to use 
*/
void activate_video()
{
    /* set the present field to be 1, the page is now valid */
    page_table[VIDEO >> MEM_OFFSET_BITS].p = 1;
}

/*
*	set_paging
*	Description:    set a page for according process
*	inputs:		    process id
*	outputs:	    nothing
*	effects:	    page directory entry in 128MB/4MB is changed
*/
void set_paging(uint32_t pid)
{
    // magic number here
    uint32_t index = 0x8000000 / 0x400000;  // 128mB / 4mB
    uint32_t physical_addr = 0x800000 + pid * 0x400000;

    /* initialize the program 4MB page */
    page_directory[index].p           = 1;    // present
    page_directory[index].r_w         = 1;
    page_directory[index].u_s         = 1;    // user mode
    page_directory[index].pwt         = 0;
    page_directory[index].pcd         = 0;
    page_directory[index].a           = 0;
    page_directory[index].reserved    = 0;
    page_directory[index].ps          = 1;    // 4mB page
    page_directory[index].g           = 0;
    page_directory[index].avail       = 0;
    page_directory[index].base_addr   = physical_addr >> MEM_OFFSET_BITS;

    /* flush TLB */
    flush_TLB();
}

/*
*	flush_TLB
*	Description:    flush TLB
*	inputs:		    nothing
*	outputs:	    nothing
*	effects:	    TLB is flushed
*/
void flush_TLB() 
{
	asm volatile(
        "movl %cr3, %eax;"
        "movl %eax, %cr3;"
    );
}
