/* TinyOS Kernel Entry Point & Low-Level Interrupt Stubs */
.code32
.global _kernel_start
.global isr_stub_table
.global irq_stub_table

/* Multiboot Header */
.set MB_MAGIC,    0x1BADB002
.set MB_FLAGS,    0x00000003
.set MB_CHECKSUM, -(MB_MAGIC + MB_FLAGS)

.section .multiboot, "a"
.align 4
.long MB_MAGIC
.long MB_FLAGS
.long MB_CHECKSUM

.section .text
_kernel_start:
    # Disable interrupts
    cli

    # Ensure segments are loaded with flat 32-bit data selector (0x10)
    mov $0x10, %cx
    mov %cx, %ds
    mov %cx, %es
    mov %cx, %fs
    mov %cx, %gs
    mov %cx, %ss

    # Set up kernel stack
    mov $_kernel_stack_top, %esp

    # Clear EFLAGS
    pushl $0
    popf

    # Zero out .bss section
    .extern _kernel_end_addr
    mov $__bss_start, %edi
    mov $_kernel_end_addr, %ecx
    sub %edi, %ecx
    xor %al, %al
    cld
    rep stosb

    # Push Multiboot info pointer (EBX) and magic (EAX)
    push %ebx
    push %eax

    # Call kernel main in C
    .extern kernel_main
    call kernel_main

    # If kernel_main returns, halt CPU
halt_loop:
    cli
    hlt
    jmp halt_loop

/* Common ISR Exception Handler Stub */
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    push $0             # Dummy error code
    push $\num          # Interrupt number
    jmp isr_common_stub
.endm

.macro ISR_ERRCODE num
.global isr\num
isr\num:
    push $\num          # Error code already pushed by CPU
    jmp isr_common_stub
.endm

/* Common IRQ Handler Stub */
.macro IRQ_STUB irq_num, int_num
.global irq\irq_num
irq\irq_num:
    push $0
    push $\int_num
    jmp irq_common_stub
.endm

# Define CPU Exceptions (0-31)
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

# Define Hardware IRQs (0-15 -> interrupts 32-47)
IRQ_STUB 0, 32
IRQ_STUB 1, 33
IRQ_STUB 2, 34
IRQ_STUB 3, 35
IRQ_STUB 4, 36
IRQ_STUB 5, 37
IRQ_STUB 6, 38
IRQ_STUB 7, 39
IRQ_STUB 8, 40
IRQ_STUB 9, 41
IRQ_STUB 10, 42
IRQ_STUB 11, 43
IRQ_STUB 12, 44
IRQ_STUB 13, 45
IRQ_STUB 14, 46
IRQ_STUB 15, 47

isr_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp           # Pass pointer to registers structure
    .extern isr_handler
    call isr_handler
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp        # Cleans up error code and interrupt number
    iret

irq_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    push %esp           # Pass pointer to registers structure
    .extern irq_handler
    call irq_handler
    add $4, %esp

    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp
    iret

# Table of ISR pointers
.section .rodata
.align 4
isr_stub_table:
    .long isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    .long isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    .long isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    .long isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

irq_stub_table:
    .long irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
    .long irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

# Kernel Stack
.section .bss
.align 16
__bss_start:
_kernel_stack_bottom:
    .skip 32768         # 32 KB kernel stack
_kernel_stack_top:
