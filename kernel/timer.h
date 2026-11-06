#ifndef TIMER_H
#define TIMER_H

#include "types.h"

void timer_init(uint32_t frequency);
uint32_t timer_get_ticks(void);
uint32_t uptime_seconds(void);
uint32_t uptime_ms(void);
void sleep_ticks(uint32_t ticks);
void sleep_ms(uint32_t ms);

#endif
