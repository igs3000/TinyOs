# TinyOS 16-bit Bootloader for Floppy / El Torito CD-ROM
.code16
.global _start
_start:
    jmp start_code
    nop

boot_drive: .byte 0

start_code:
    cli
    xor %ax, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    mov $0x7C00, %sp
    sti

    mov %dl, boot_drive

    # Initialize COM1 serial port at 0x3F8 (115200 baud, 8N1)
    mov $0x3F9, %dx
    mov $0x00, %al      # Disable interrupts
    out %al, %dx
    mov $0x3FB, %dx
    mov $0x80, %al      # Enable DLAB (set baud rate divisor)
    out %al, %dx
    mov $0x3F8, %dx
    mov $0x01, %al      # Divisor low byte (115200 / 1 = 115200 baud)
    out %al, %dx
    mov $0x3F9, %dx
    mov $0x00, %al      # Divisor high byte
    out %al, %dx
    mov $0x3FB, %dx
    mov $0x03, %al      # 8 bits, no parity, one stop bit
    out %al, %dx
    mov $0x3FA, %dx
    mov $0xC7, %al      # Enable FIFO, clear them, 14-byte threshold
    out %al, %dx
    mov $0x3FC, %dx
    mov $0x0B, %al      # IRQs enabled, RTS/DSR set
    out %al, %dx

    # Print booting message
    mov $msg_boot, %si
    call print_str

    # Enable A20 gate via BIOS INT 15h, AX=2401h first
    mov $0x2401, %ax
    int $0x15

    # Fast A20 gate fallback via System Control Port A (0x92)
    inb $0x92, %al
    orb $0x02, %al
    andb $0xFE, %al     # Do not trigger fast reset
    outb %al, $0x92

    # Load kernel from disk: 128 sectors (64 KB) starting at LBA 1
    # into memory at 0x1000:0000 (physical 0x10000)
    mov $1, %ax          # LBA 1 (Sector 2 on floppy)
    mov $0x1000, %bx
    mov %bx, %es
    mov $128, %si        # Number of sectors to load (128 * 512 = 64KB)

load_kernel_loop:
    call read_one_sector
    # Advance ES by 512 bytes = 0x20 paragraphs
    mov %es, %bx
    add $0x20, %bx
    mov %bx, %es
    inc %ax              # Next LBA
    dec %si              # Remaining count
    jnz load_kernel_loop

    mov $msg_ready, %si
    call print_str

    # Disable interrupts before entering 32-bit protected mode
    cli

    # Load Global Descriptor Table (GDT)
    lgdt gdt_descriptor

    # Set PE (Protection Enable) bit in CR0
    mov %cr0, %eax
    or $1, %eax
    mov %eax, %cr0

    # Far jump to 32-bit code segment (0x08)
    ljmp $0x08, $protected_mode_entry

# Read 1 sector via BIOS INT 13h.
# Inputs: AX = LBA, ES:0000 = destination buffer
read_one_sector:
    push %ax
    push %bx
    push %cx
    push %dx

    push %ax
    # Sector = (LBA % 18) + 1
    xor %dx, %dx
    mov $18, %bx
    div %bx
    inc %dx
    mov %dl, %cl        # CL = Sector (1-18)

    # Cylinder = (LBA / 18) / 2
    # Head     = (LBA / 18) % 2
    xor %dx, %dx
    mov $2, %bx
    div %bx
    mov %dl, %dh        # DH = Head (0-1)
    mov %al, %ch        # CH = Cylinder

    mov boot_drive, %dl # DL = Boot drive
    pop %ax

    mov $3, %di         # 3 retries
retry_read:
    mov $0x0201, %ax    # AH=02 (read), AL=01 (1 sector)
    xor %bx, %bx        # Buffer offset = 0
    int $0x13
    jnc read_success

    # Reset drive on error
    xor %ax, %ax
    mov boot_drive, %dl
    int $0x13
    dec %di
    jnz retry_read

    # Disk read failure
    mov $msg_disk_err, %si
    call print_str
halt_cpu:
    hlt
    jmp halt_cpu

read_success:
    pop %dx
    pop %cx
    pop %bx
    pop %ax
    ret

print_str:
    push %ax
    push %dx
print_loop:
    lodsb
    test %al, %al
    jz print_done
    # Output to COM1 serial
    mov $0x3F8, %dx
    out %al, %dx
    # Output to VGA screen
    mov $0x0E, %ah
    int $0x10
    jmp print_loop
print_done:
    pop %dx
    pop %ax
    ret

msg_boot:     .ascii "TinyOS: Booting from ISO CD-ROM...\r\n\0"
msg_ready:    .ascii "TinyOS: Kernel loaded. Switching to 32-bit Protected Mode...\r\n\0"
msg_disk_err: .ascii "TinyOS: Disk read error!\r\n\0"

# GDT (Global Descriptor Table)
.align 8
gdt_start:
    # 0x00: Null descriptor
    .quad 0x0000000000000000

    # 0x08: 32-bit Code Descriptor (Base 0, Limit 4GB, Exec/Read, Ring 0)
    .word 0xFFFF        # Limit 15:0
    .word 0x0000        # Base 15:0
    .byte 0x00          # Base 23:16
    .byte 0x9A          # Access: Present, Ring 0, Code, Exec/Read
    .byte 0xCF          # Flags: 4KB granularity, 32-bit mode + Limit 19:16
    .byte 0x00          # Base 31:24

    # 0x10: 32-bit Data Descriptor (Base 0, Limit 4GB, Read/Write, Ring 0)
    .word 0xFFFF        # Limit 15:0
    .word 0x0000        # Base 15:0
    .byte 0x00          # Base 23:16
    .byte 0x92          # Access: Present, Ring 0, Data, Read/Write
    .byte 0xCF          # Flags: 4KB granularity, 32-bit mode + Limit 19:16
    .byte 0x00          # Base 31:24
gdt_end:

gdt_descriptor:
    .word gdt_end - gdt_start - 1
    .long gdt_start

# 32-Bit Protected Mode Trampoline
.code32
protected_mode_entry:
    # Set data segment registers to 0x10 (Data Descriptor)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # Set up stack at 0x90000 (conventional memory below VGA buffer)
    mov $0x90000, %esp

    # Relocate kernel from 0x10000 to standard 1MB mark (0x100000)
    mov $0x10000, %esi
    mov $0x100000, %edi
    mov $16384, %ecx     # 16384 dwords = 65536 bytes
    cld
    rep movsl

    # Jump to relocated kernel entry point at 0x100000
    mov $0x100000, %eax
    jmp *%eax

.code16
.fill 510 - (. - _start), 1, 0
.word 0xAA55
