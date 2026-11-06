#include "timer.h"
#include "idt.h"
#include "io.h"

static volatile uint32_t system_ticks = 0;
static uint32_t timer_hz = 100;

static void timer_callback(registers_t* regs) {
    (void)regs;
    system_ticks++;
}

void timer_init(uint32_t frequency) {
    timer_hz = frequency;
    register_interrupt_handler(32, timer_callback);

    // Divisor must fit into 16 bits
    uint32_t divisor = 1193182 / frequency;
    if (divisor > 65535) divisor = 65535;

    // Send command byte (channel 0, access mode: lobyte/hibyte, mode 3: square wave)
    outb(0x43, 0x36);

    // Send frequency divisor
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t timer_get_ticks(void) {
    return system_ticks;
}

uint32_t uptime_seconds(void) {
    return system_ticks / timer_hz;
}

uint32_t uptime_ms(void) {
    return (system_ticks * 1000) / timer_hz;
}

void sleep_ticks(uint32_t ticks) {
    uint32_t end = system_ticks + ticks;
    while (system_ticks < end) {
        hlt();
    }
}

void sleep_ms(uint32_t ms) {
    uint32_t ticks = (ms * timer_hz) / 1000;
    if (ticks == 0 && ms > 0) ticks = 1;
    sleep_ticks(ticks);
}
