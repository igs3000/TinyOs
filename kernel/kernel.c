#include "types.h"
#include "console.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include "fs.h"
#include "shell.h"
#include "io.h"

void kernel_main(uint32_t magic, void* mb_info) {
    (void)magic;
    (void)mb_info;

    // Initialize console (VGA + Serial COM1)
    console_init();
    console_clear();

    // Print boot banner
    console_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printf("================================================================================\n");
    printf("                     TinyOS - Minimal 32-bit x86 OS                             \n");
    printf("================================================================================\n");

    console_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("[ OK ] Initializing Hardware Abstraction Layer...\n");

    // Initialize IDT & remap PIC
    idt_init();
    printf("[ OK ] IDT loaded & 8259 PIC remapped (Vectors 32-47)\n");

    // Initialize PIT Timer at 100 Hz
    timer_init(100);
    printf("[ OK ] PIT Timer initialized at 100 Hz\n");

    // Initialize PS/2 Keyboard
    keyboard_init();
    printf("[ OK ] PS/2 Keyboard driver active\n");

    // Enable CPU Interrupts
    sti();
    printf("[ OK ] Hardware interrupts enabled\n");

    // Initialize TinyFS RAM Filesystem
    fs_init();
    printf("[ OK ] TinyFS mounted with default files (%d files loaded)\n", fs_count());

    console_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    printf("\nWelcome to TinyOS!\n");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf("Type ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("'help'");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf(" to list commands, ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("'ls'");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf(" to view files, or ");
    console_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("'cat readme.txt'");
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf(" for system info.\n\n");

    // Launch terminal shell
    shell_init();
    shell_run();

    // In case shell terminates
    cli();
    while (1) {
        hlt();
    }
}
