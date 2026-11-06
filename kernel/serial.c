#include "serial.h"
#include "io.h"

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Disable interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x01);    // Set divisor to 1 (lo byte) 115200 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

bool serial_has_char(void) {
    return (inb(COM1_PORT + 5) & 0x01) != 0;
}

char serial_getchar(void) {
    while (!serial_has_char());
    return (char)inb(COM1_PORT);
}

static bool is_transmit_empty(void) {
    return (inb(COM1_PORT + 5) & 0x20) != 0;
}

void serial_putc(char c) {
    if (c == '\n') {
        while (!is_transmit_empty());
        outb(COM1_PORT, '\r');
    }
    while (!is_transmit_empty());
    outb(COM1_PORT, (uint8_t)c);
}

void serial_puts(const char* s) {
    while (*s) {
        serial_putc(*s++);
    }
}
