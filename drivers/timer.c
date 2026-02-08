#include "timer.h"
#include "pic.h"
#include <stdint.h>

/* PIT (Programmable Interval Timer) */
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43
#define PIT_FREQUENCY 1193182

/* I/O port operations */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* Timer ticks */
static volatile uint64_t timer_ticks = 0;

/* Timer interrupt handler (called from IRQ0) */
void timer_interrupt_handler(void) {
    timer_ticks++;
}

/* Initialize timer */
void timer_init(uint32_t frequency) {
    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;

    /* Send command byte */
    outb(PIT_COMMAND, 0x36);

    /* Send divisor */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    timer_ticks = 0;
}

/* Get timer ticks */
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

/* Wait for specified milliseconds */
void timer_wait(uint32_t ms) {
    uint64_t target = timer_ticks + ms;
    while (timer_ticks < target) {
        __asm__ volatile ("hlt");
    }
}
