#include "idt.h"
#include <stdint.h>

/* Driver interrupt handlers */
extern void timer_interrupt_handler(void);
extern void keyboard_interrupt_handler(void);
extern void mouse_interrupt_handler(void);

/* Registers saved by interrupt */
struct registers {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

/* ISR handler */
void isr_handler(struct registers *regs) {
    (void)regs;
    /* Handle CPU exceptions */
    /* For now, just ignore them */
}

/* IRQ handler */
void irq_handler(struct registers *regs) {
    /* Call driver interrupt handlers */
    switch (regs->int_no) {
        case 32:  /* IRQ0 - Timer */
            timer_interrupt_handler();
            break;
        case 33:  /* IRQ1 - Keyboard */
            keyboard_interrupt_handler();
            break;
        case 44:  /* IRQ12 - Mouse */
            mouse_interrupt_handler();
            break;
    }

    /* Send EOI to PIC */
    if (regs->int_no >= 40) {
        /* Send EOI to slave PIC */
        __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0xA0));
    }
    /* Send EOI to master PIC */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
}
