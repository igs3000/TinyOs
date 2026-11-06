# TinyOS Makefile
CC      = gcc
AS      = as
LD      = ld
OBJCOPY = objcopy
PYTHON  = python3

CFLAGS  = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin \
          -Wall -Wextra -O2 -nostdlib -Ikernel
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

C_SRCS  = kernel/kernel.c \
          kernel/console.c \
          kernel/vga.c \
          kernel/serial.c \
          kernel/string.c \
          kernel/idt.c \
          kernel/timer.c \
          kernel/keyboard.c \
          kernel/rtc.c \
          kernel/fs.c \
          kernel/shell.c

C_OBJS  = $(C_SRCS:.c=.o)
S_OBJS  = kernel/entry.o

all: tinyos.iso

# 16-bit Bootloader
boot/boot.o: boot/boot.s
	$(AS) $(ASFLAGS) -o $@ $<

boot/boot.bin: boot/boot.o
	$(LD) -m elf_i386 --oformat binary -Ttext 0x7C00 -o $@ $<

# 32-bit Kernel
kernel/entry.o: kernel/entry.s
	$(AS) $(ASFLAGS) -o $@ $<

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/kernel.elf: $(S_OBJS) $(C_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(S_OBJS) $(C_OBJS)

kernel/kernel.bin: kernel/kernel.elf
	$(OBJCOPY) -O binary $< $@

# Bootable ISO
tinyos.iso: boot/boot.bin kernel/kernel.bin tools/make_iso.py
	$(PYTHON) tools/make_iso.py boot/boot.bin kernel/kernel.bin $@

# Run with QEMU in GUI window
run: tinyos.iso
	qemu-system-x86_64 -cdrom tinyos.iso -boot d

# Run with QEMU in terminal / serial mode
run-serial: tinyos.iso
	qemu-system-x86_64 -cdrom tinyos.iso -boot d -nographic

# Direct kernel boot via Multiboot (alternative)
run-kernel: kernel/kernel.elf
	qemu-system-x86_64 -kernel kernel/kernel.elf

clean:
	rm -f boot/*.o boot/*.bin kernel/*.o kernel/*.elf kernel/*.bin tinyos.iso

.PHONY: all run run-serial run-kernel clean
