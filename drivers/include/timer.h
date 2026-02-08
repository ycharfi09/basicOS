#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Timer functions */
void timer_init(uint32_t frequency);
uint64_t timer_get_ticks(void);
void timer_wait(uint32_t ms);

#endif /* TIMER_H */
