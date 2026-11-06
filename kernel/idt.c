#include "idt.h"
#include "io.h"
#include "console.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

static isr_t interrupt_handlers[256];

extern uint32_t isr_stub_table[32];
extern uint32_t irq_stub_table[16];

static const char* exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security Exception",
    "Reserved"
};

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low  = (uint16_t)(base & 0xFFFF);
    idt[num].selector    = sel;
    idt[num].zero        = 0;
    idt[num].type_attr   = flags;
    idt[num].offset_high = (uint16_t)((base >> 16) & 0xFFFF);
}

void register_interrupt_handler(uint8_t n, isr_t handler) {
    interrupt_handlers[n] = handler;
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

static void pic_remap(void) {
    // ICW1: Start initialization in cascade mode
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();

    // ICW2: Master PIC vector offset (32), Slave PIC offset (40)
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();

    // ICW3: Tell Master there is a slave at IRQ2, tell Slave its cascade identity
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();

    // ICW4: 8086 mode
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    // Restore masks, enabling IRQ0 (timer) and IRQ1 (keyboard)
    outb(0x21, 0xFC); // 1111 1100b
    outb(0xA1, 0xFF); // 1111 1111b
}

void isr_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](regs);
    } else {
        console_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
        printf("\n================ [ KERNEL PANIC ] ================\n");
        printf("Unhandled CPU Exception: %d (%s)\n", regs->int_no,
               (regs->int_no < 32) ? exception_messages[regs->int_no] : "Unknown");
        printf("Error Code: 0x%x  EIP: 0x%x  CS: 0x%x  EFLAGS: 0x%x\n",
               regs->err_code, regs->eip, regs->cs, regs->eflags);
        printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
               regs->eax, regs->ebx, regs->ecx, regs->edx);
        printf("ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
               regs->esi, regs->edi, regs->ebp, regs->esp);
        printf("System halted.\n");
        cli();
        while (1) hlt();
    }
}

void irq_handler(registers_t* regs) {
    if (interrupt_handlers[regs->int_no]) {
        interrupt_handlers[regs->int_no](regs);
    }
    pic_send_eoi((uint8_t)(regs->int_no - 32));
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        interrupt_handlers[i] = NULL;
    }

    // Set exception handlers (0-31)
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x08, 0x8E);
    }

    // Remap the PIC
    pic_remap();

    // Set IRQ handlers (32-47)
    for (int i = 0; i < 16; i++) {
        idt_set_gate(32 + i, irq_stub_table[i], 0x08, 0x8E);
    }

    // Load IDT into CPU
    __asm__ volatile ("lidt %0" : : "m"(idtp));
}
