#include "pic.h"
#include <stdint.h>

/* PIC (Programmable Interrupt Controller) ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* PIC commands */
#define ICW1_INIT    0x11
#define ICW4_8086    0x01
#define PIC_EOI      0x20

/* I/O port operations */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

/* Initialize PIC */
void pic_init(void) {
    /* Start initialization sequence */
    outb(PIC1_COMMAND, ICW1_INIT);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT);
    io_wait();

    /* Set vector offsets */
    outb(PIC1_DATA, 0x20);  /* Master PIC: IRQ 0-7 -> INT 32-39 */
    io_wait();
    outb(PIC2_DATA, 0x28);  /* Slave PIC: IRQ 8-15 -> INT 40-47 */
    io_wait();

    /* Tell Master PIC about Slave PIC at IRQ2 */
    outb(PIC1_DATA, 0x04);
    io_wait();
    /* Tell Slave PIC its cascade identity */
    outb(PIC2_DATA, 0x02);
    io_wait();

    /* Set 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /* Enable needed interrupts: timer (IRQ0), keyboard (IRQ1), cascade (IRQ2) */
    outb(PIC1_DATA, 0xF8);
    /* Enable needed interrupts: mouse (IRQ12 = slave IRQ4) */
    outb(PIC2_DATA, 0xEF);
}

/* Send End Of Interrupt */
void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

/* Disable PIC */
void pic_disable(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
