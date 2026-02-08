#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/* PIC (Programmable Interrupt Controller) */
void pic_init(void);
void pic_send_eoi(uint8_t irq);
void pic_disable(void);

#endif /* PIC_H */
