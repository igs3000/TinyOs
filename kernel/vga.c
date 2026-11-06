#include "vga.h"
#include "io.h"
#include "string.h"

static volatile uint16_t* const VGA_MEMORY = (volatile uint16_t*)0xB8000;
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t current_color = 0x07; // Light grey on black

static inline uint16_t vga_entry(unsigned char ch, uint8_t color) {
    return (uint16_t)ch | ((uint16_t)color << 8);
}

static void update_hardware_cursor(void) {
    uint16_t pos = (uint16_t)vga_row * VGA_WIDTH + vga_col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);
    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void vga_init(void) {
    current_color = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_GREY;
    vga_enable_cursor(14, 15);
    vga_clear();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    current_color = (bg << 4) | (fg & 0x0F);
}

uint8_t vga_get_color(void) {
    return current_color;
}

void vga_clear(void) {
    uint16_t blank = vga_entry(' ', current_color);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_MEMORY[i] = blank;
    }
    vga_row = 0;
    vga_col = 0;
    update_hardware_cursor();
}

static void vga_scroll(void) {
    uint16_t blank = vga_entry(' ', current_color);
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            VGA_MEMORY[y * VGA_WIDTH + x] = VGA_MEMORY[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
    vga_row = VGA_HEIGHT - 1;
}

void vga_putc(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        int next_tab = (vga_col + 4) & ~3;
        while (vga_col < next_tab && vga_col < VGA_WIDTH) {
            VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', current_color);
            vga_col++;
        }
    } else if (c == '\b') {
        vga_backspace();
        return;
    } else {
        VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = vga_entry((unsigned char)c, current_color);
        vga_col++;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    }

    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
    update_hardware_cursor();
}

void vga_puts(const char* s) {
    while (*s) {
        vga_putc(*s++);
    }
}

void vga_backspace(void) {
    if (vga_col > 0) {
        vga_col--;
        VGA_MEMORY[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', current_color);
        update_hardware_cursor();
    }
}

void vga_set_cursor(uint8_t row, uint8_t col) {
    if (row < VGA_HEIGHT) vga_row = row;
    if (col < VGA_WIDTH)  vga_col = col;
    update_hardware_cursor();
}

void vga_get_cursor(uint8_t* row, uint8_t* col) {
    if (row) *row = vga_row;
    if (col) *col = vga_col;
}

void vga_write_at(char c, uint8_t color, uint8_t row, uint8_t col) {
    if (row < VGA_HEIGHT && col < VGA_WIDTH) {
        VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry((unsigned char)c, color);
    }
}
