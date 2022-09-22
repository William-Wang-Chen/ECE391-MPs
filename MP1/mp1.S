
.data					# section declaration

        # Useful offset constants for accessing members of a 
        # struct mp1_blink_struct structure
        LOCATION   = 0    
        ON_CHAR    = 2
        OFF_CHAR   = 3 
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12

        STRUCT_SIZE = 16

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0

.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX
mp1_poke:
        
	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret

/*
void mp1 rtc tasklet (unsigned long);
Interface: C-convention
Input: no input
Output: no output
Registers: Clobbers eax ecx
*/
mp1_rtc_tasklet:
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        // set pointers
        movl mp1_list_head, %ebx
check_loop:
        // check whether at the end of the list
        cmpl $0, %ebx
        je tasklet_finish
        // decrease countdown
        decw COUNTDOWN(%ebx)
        // if countdown == 0
        cmpw $0, COUNTDOWN(%ebx)
        jne next_check_loop
        // if status == 0
        cmpw $0, STATUS(%ebx)
        je off_status
on_status:
        // status now is 1
        // call mp1_poke
        xor %eax, %eax
        movw LOCATION(%ebx), %ax
        addl %eax, %eax
        movb OFF_CHAR(%ebx), %cl
        call mp1_poke
        // change status
        movw $0, STATUS(%ebx)
        movw OFF_LENGTH(%ebx), %cx
        movw %cx, COUNTDOWN(%ebx)
        jmp next_check_loop
off_status:
        // status now is 0
        // call mp1_poke
        xor %eax, %eax
        movw LOCATION(%ebx), %ax
        addl %eax, %eax
        movb ON_CHAR(%ebx), %cl
        call mp1_poke
        // change status
        movw $1, STATUS(%ebx)
        movw ON_LENGTH(%ebx), %cx
        movw %cx, COUNTDOWN(%ebx)
        jmp next_check_loop
next_check_loop:
        movl NEXT(%ebx), %ebx
        jmp check_loop
tasklet_finish:
        leal -12(%ebp), %esp
        popl %edi
        popl %esi
        popl %ebx
        leave
        ret

/*
int mp1_ioctl (unsigned long arg, unsigned long cmd);
Interface: Stack-based arguments (not C-style)
Input:  unsigned long arg - arg for function in jump table
        unsigned long cmd - index of functions in jump table
Output: eax
        -1 if error
        jump table function output if no error
Registers: Clobbers eax ecx
*/
mp1_ioctl:
        movl 8(%esp), %ecx
        // cmd validation
        cmpl $3, %ecx
        jg cmd_error
        cmpl $0, %ecx
        jl cmd_error
        // jump to corresponding function
        jmp *jump_table(, %ecx, 4)
cmd_error:
        movl $-1, %eax
        ret

/*
int mp1 ioctl add(unsigned long arg) 
Interface: Stack-based arguments (C-convention)
Input:  unsigned long arg - user space pointer pointing to a blink structure
Output: eax
        -1 if error
        0 if success
Registers: Clobbers eax ecx
*/
mp1_ioctl_add:
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        // if pointer to structure is NULL
        cmpl $0, 8(%ebp)
        je add_error_return
        // call mp1_malloc
        pushl $STRUCT_SIZE
        call mp1_malloc
        addl $4, %esp
        // get return value
        movl %eax, %ebx
        // if NULL returned, error
        cmpl $0, %ebx
        je add_error_return
        // call mp1_copy_from_user
        pushl $STRUCT_SIZE
        pushl 8(%ebp)
        pushl %ebx
        call mp1_copy_from_user
        addl $12, %esp
        // if not succesfully copied
        cmpl $0, %eax
        jne add_error
        // if location is outside the valid range (0~1999)
        cmpw $0, LOCATION(%ebx)
        jl add_error
        cmpw $1999, LOCATION(%ebx)
        jg add_error
        // set field
        movw ON_LENGTH(%ebx), %cx
        movw %cx, COUNTDOWN(%ebx)
        movw $1, STATUS(%ebx)
        // link list
        movl mp1_list_head, %ecx
        movl %ecx, NEXT(%ebx)
        movl %ebx, mp1_list_head
        // call mp1_poke
        xor %eax, %eax
        movw LOCATION(%ebx), %ax
        addl %eax, %eax
        movb ON_CHAR(%ebx), %cl
        call mp1_poke
        // succesfully add
        movl $0, %eax
        jmp add_finish
add_error:
        // free the memory
        pushl %ebx
        call mp1_free
        addl $4, %esp
add_error_return:
        // return -1
        movl $-1, %eax
        jmp add_finish
add_finish:
        leal -12(%ebp), %esp
        popl %edi
        popl %esi
        popl %ebx
        leave
        ret

/*
int mp1 ioctl remove(unsigned long arg)
Interface: Stack-based arguments (C-convention)
Input:  unsigned long arg - location of the structure which needs to be removed
Output: eax
        -1 if error
        0 if success
Registers: Clobbers eax edx ecx
*/
mp1_ioctl_remove:
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        // if location is outside the valid range (0~1999)
        cmpw $0, 8(%ebp)
        jl remove_error_return
        cmpw $1999, 8(%ebp)
        jg remove_error_return
        // call searched_linked_list
        pushl 8(%ebp)
        call search_linked_list
        addl $4, %esp
        // if not find
        cmpl $0, %eax
        je remove_error_return
        // if it is the first element
        cmpl $0, %edx
        je remove_first
        // remove the node (not first)
        movl NEXT(%eax), %ebx
        movl %ebx, NEXT(%edx)
        jmp remove_free_node
remove_first:
        movl NEXT(%eax), %ebx
        movl %ebx, mp1_list_head
        jmp remove_free_node
remove_free_node:
        // free the node
        pushl %eax
        call mp1_free
        addl $4, %esp
        // return 0
        movl $0, %eax
        jmp remove_finish
remove_error_return:
        // return -1
        movl $-1, %eax
        jmp remove_finish
remove_finish:
        leal -12(%ebp), %esp
        popl %edi
        popl %esi
        popl %ebx
        leave
        ret

/*
int mp1 ioctl find(unsigned long arg)
Interface: Stack-based arguments (C-convention)
Input:  unsigned long arg - user space pointer pointing to a blink structure
Output: eax
        -1 if error
        0 if success
        *arg
        finded structure in the memory pointed by arg
Registers: Clobbers eax edx ecx
*/
mp1_ioctl_find:
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        // check whether it is a valid pointer
        cmpl $0, 8(%ebp)
        je find_error_return
        // call mp1_malloc
        pushl $STRUCT_SIZE
        call mp1_malloc
        addl $4, %esp
        // get return value
        movl %eax, %ebx
        // if NULL returned, error
        cmpl $0, %ebx
        je find_error_return
        // call mp1_copy_from_user
        pushl $STRUCT_SIZE
        pushl 8(%ebp)
        pushl %ebx
        call mp1_copy_from_user
        addl $12, %esp
        // if not succesfully copied
        cmpl $0, %eax
        jne find_error
        // if location is outside the valid range (0~1999)
        cmpw $0, LOCATION(%ebx)
        jl find_error
        cmpw $1999, LOCATION(%ebx)
        jg find_error
        // extract LOCATION
        xor %ecx, %ecx
        movw LOCATION(%ebx), %cx
        // free the temp structure
        // caller save
        pushl %ecx
        // push arg
        pushl %ebx
        call mp1_free
        addl $4, %esp
        // restore ecx
        popl %ecx
        // call searched_linked_list
        pushl %ecx
        call search_linked_list
        addl $4, %esp
        // if not found
        cmpl $0, %eax
        je find_error_return
        // call mp1 copy to user
        pushl $STRUCT_SIZE
        pushl %eax
        pushl 8(%ebp)
        call mp1_copy_to_user
        addl $12, %esp
        // if not succesfully copied
        cmpl $0, %eax
        jne find_error_return
        // succesfully find and copied
        movl $0, %eax
        jmp find_finish
find_error:
        // free the memory
        pushl %ebx
        call mp1_free
        addl $4, %esp
find_error_return:
        // return -1
        movl $-1, %eax
        jmp find_finish
find_finish:
        leal -12(%ebp), %esp
        popl %edi
        popl %esi
        popl %ebx
        leave
        ret

/*
int mp1 ioctl sync(unsigned long arg)
Interface: Stack-based arguments (C-convention)
Input:  unsigned long arg - 
                two two-byte unsigned short integers packed in a long arg
                The ﬁrst integer is stored in the upper 16 bits of arg
                The second integer is stored in the lower 16 bits
                Two locations which need to be synchronized
Output: eax
        -1 if error
        0 if success
Registers: Clobbers eax edx ecx
*/
mp1_ioctl_sync:
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        // get first location
        // clear low bits then shift
        movl 8(%ebp), %ecx
        xorw %cx, %cx
        shrl $16, %ecx
        // if location is outside the valid range (0~1999)
        cmpw $0, %cx
        jl sync_error_return
        cmpw $1999, %cx
        jg sync_error_return
        // find first structure
        pushl %ecx
        call search_linked_list
        addl $4, %esp
        // if not found
        cmpl $0, %eax
        je sync_error_return
        // pointer saved in esi
        movl %eax, %esi
        // get second location
        // clear high bits by anding with 0x0000FFFF
        movl 8(%ebp), %ecx
        andl $0x0000FFFF, %ecx
        // if location is outside the valid range (0~1999)
        cmpw $0, %cx
        jl sync_error_return
        cmpw $1999, %cx
        jg sync_error_return
        // find second structure
        pushl %ecx
        call search_linked_list
        addl $4, %esp
        // if not found
        cmpl $0, %eax
        je sync_error_return
        // pointer saved in edi
        movl %eax, %edi
        // copy elements
        // copy both on-length and off-length
        movl ON_LENGTH(%esi), %ecx
        movl %ecx, ON_LENGTH(%edi)
        // copy both countdown and status
        movl COUNTDOWN(%esi), %ecx
        movl %ecx, COUNTDOWN(%edi)
        // check status
        cmpw $0, STATUS(%edi)
        je sync_off_status
sync_on_status:
        movb ON_CHAR(%edi), %cl
        jmp sync_call_poke
sync_off_status:
        movb OFF_CHAR(%edi), %cl
        jmp sync_call_poke
sync_call_poke:
        // call mp1_poke
        xor %eax, %eax
        movw LOCATION(%edi), %ax
        addl %eax, %eax
        call mp1_poke
sync_success:
        // succesfully sync
        movl $0, %eax
        jmp sync_finish
sync_error_return:
        // return -1
        movl $-1, %eax
        jmp sync_finish
sync_finish:
        leal -12(%ebp), %esp
        popl %edi
        popl %esi
        popl %ebx
        leave
        ret

/*
void* search_linked_list(unsigned long location);
Interface: C-convention
Input: unsigned long location in stack
Output: eax, ecx
        the pointer of the coorsponding node in eax
        the pointer of the previous node in edx
        if it is the first node
                return NULL in edx
        if no such node
                return NULL in eax
                return NULL in edx
Registers: Clobbers eax, edx, ecx
*/
search_linked_list:
        pushl %ebp
        movl %esp, %ebp
        pushl %ebx
        pushl %esi
        pushl %edi
        // set the pointer
        movl mp1_list_head, %ebx
        xorl %edx, %edx
search_loop:
        // check whether at the end of the list
        cmpl $0, %ebx
        je search_finish
        // if location == struct->location
        movl 8(%ebp), %ecx
        cmpw %cx, LOCATION(%ebx)
        je search_finish
        // search next node
        movl %ebx, %edx
        movl NEXT(%ebx), %ebx
        jmp search_loop
search_finish:
        movl %ebx, %eax
        leal -12(%ebp), %esp
        popl %edi
        popl %esi
        popl %ebx
        leave
        ret

jump_table:
.long mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync

.end
