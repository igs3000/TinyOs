#ifndef IDT_H
#define IDT_H

#include "types.h"

typedef void (*isr_t)(registers_t*);

void idt_init(void);
void register_interrupt_handler(uint8_t n, isr_t handler);
void pic_send_eoi(uint8_t irq);

#endif
