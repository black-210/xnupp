.section .text
.global long_mode_start
.type long_mode_start,@function

long_mode_start:
    cli

    /* Enable PAE */
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4

    /* Load boot page table */
    leal boot_pml4, %eax
    movl %eax, %cr3

    /* Enable Long Mode */
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr

    /* Enable paging */
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0

    /* Reload 64-bit code segment */
    ljmp $0x08, $long_mode_entry

.code64
long_mode_entry:
    xorq %rax, %rax
    xorq %rbx, %rbx
    xorq %rcx, %rcx
    xorq %rdx, %rdx

    movq $stack_top64, %rsp
    call boot_entry

1:
    cli
    hlt
    jmp 1b

.section .bss
.align 4096

boot_pml4:
    .skip 4096

boot_pdpt:
    .skip 4096

boot_pd:
    .skip 4096

.align 16
stack64:
    .skip 16384
stack_top64:
