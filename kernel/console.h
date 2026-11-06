#ifndef CONSOLE_H
#define CONSOLE_H

#include "types.h"
#include "vga.h"

void console_init(void);
void console_putc(char c);
void console_puts(const char* s);
void console_clear(void);
void console_set_color(uint8_t fg, uint8_t bg);
uint8_t console_get_color(void);
void console_backspace(void);

bool console_has_char(void);
char console_getchar(void);

int printf(const char* fmt, ...);

#endif
