#ifndef SERIAL_H
#define SERIAL_H

#include "types.h"

#define COM1_PORT 0x3F8

void serial_init(void);
void serial_putc(char c);
void serial_puts(const char* s);
bool serial_has_char(void);
char serial_getchar(void);

#endif
