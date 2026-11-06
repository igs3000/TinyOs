#include "console.h"
#include "vga.h"
#include "serial.h"
#include "string.h"
#include "io.h"

// Forward declaration from keyboard
extern bool keyboard_has_key(void);
extern char keyboard_getchar(void);

static const int ansi_fg_map[16] = {
    30, 34, 32, 36, 31, 35, 33, 37,
    90, 94, 92, 96, 91, 95, 93, 97
};

static const int ansi_bg_map[16] = {
    40, 44, 42, 46, 41, 45, 43, 47,
    100, 104, 102, 106, 101, 105, 103, 107
};

void console_init(void) {
    vga_init();
    serial_init();
    console_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

void console_putc(char c) {
    vga_putc(c);
    serial_putc(c);
}

void console_puts(const char* s) {
    vga_puts(s);
    serial_puts(s);
}

void console_clear(void) {
    vga_clear();
    serial_puts("\033[2J\033[H");
}

void console_set_color(uint8_t fg, uint8_t bg) {
    vga_set_color(fg, bg);
    char seq[32];
    sprintf(seq, "\033[%d;%dm", ansi_fg_map[fg & 0x0F], ansi_bg_map[bg & 0x0F]);
    serial_puts(seq);
}

uint8_t console_get_color(void) {
    return vga_get_color();
}

void console_backspace(void) {
    vga_backspace();
    serial_puts("\b \b");
}

bool console_has_char(void) {
    return keyboard_has_key() || serial_has_char();
}

char console_getchar(void) {
    while (1) {
        if (keyboard_has_key()) {
            return keyboard_getchar();
        }
        if (serial_has_char()) {
            char c = serial_getchar();
            if (c == '\r') c = '\n'; // Normalize carriage return to newline
            if (c == 0x7F) c = '\b'; // Normalize backspace
            return c;
        }
        hlt(); // Sleep CPU until next hardware interrupt (timer, keyboard, or serial)
    }
}

int printf(const char* fmt, ...) {
    char buf[1024];
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    __builtin_va_end(args);
    console_puts(buf);
    return ret;
}
