#include "mouse.h"
#include <stdint.h>
#include <stdbool.h>

/* Mouse ports */
#define MOUSE_DATA_PORT 0x60
#define MOUSE_COMMAND_PORT 0x64

/* Mouse state */
static struct mouse_state current_mouse_state = {0, 0, false, false, false};
static uint8_t mouse_cycle = 0;
static uint8_t mouse_bytes[3];

/* I/O port operations */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Wait for mouse */
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(MOUSE_COMMAND_PORT) & 1) == 1) {
                return;
            }
        }
    } else {
        while (timeout--) {
            if ((inb(MOUSE_COMMAND_PORT) & 2) == 0) {
                return;
            }
        }
    }
}

/* Write to mouse */
static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0xD4);
    mouse_wait(1);
    outb(MOUSE_DATA_PORT, data);
}

/* Read from mouse */
static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(MOUSE_DATA_PORT);
}

/* Initialize mouse */
void mouse_init(void) {
    /* Enable auxiliary device */
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0xA8);

    /* Enable interrupts */
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0x20);
    uint8_t status = mouse_read() | 2;
    mouse_wait(1);
    outb(MOUSE_COMMAND_PORT, 0x60);
    mouse_wait(1);
    outb(MOUSE_DATA_PORT, status);

    /* Use default settings */
    mouse_write(0xF6);
    mouse_read();

    /* Enable data reporting */
    mouse_write(0xF4);
    mouse_read();

    /* Reset state */
    current_mouse_state.x = 512;
    current_mouse_state.y = 384;
    current_mouse_state.left_button = false;
    current_mouse_state.right_button = false;
    current_mouse_state.middle_button = false;
    mouse_cycle = 0;
}

/* Mouse interrupt handler (called from IRQ12) */
void mouse_interrupt_handler(void) {
    uint8_t data = inb(MOUSE_DATA_PORT);
    
    /* Verify first byte alignment - bit 3 must be set in byte 0 */
    if (mouse_cycle == 0 && !(data & 0x08)) {
        return;  /* Discard and resynchronize */
    }

    mouse_bytes[mouse_cycle] = data;
    mouse_cycle++;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        /* Discard packet if overflow bits are set */
        if (mouse_bytes[0] & 0xC0) {
            return;
        }

        /* Parse mouse packet with sign extension from status byte */
        int delta_x = mouse_bytes[1];
        int delta_y = mouse_bytes[2];
        if (mouse_bytes[0] & 0x10) delta_x -= 256;
        if (mouse_bytes[0] & 0x20) delta_y -= 256;

        /* Update mouse state */
        current_mouse_state.x += delta_x;
        current_mouse_state.y -= delta_y;  /* Invert Y axis */

        /* Clamp to screen bounds */
        if (current_mouse_state.x < 0) current_mouse_state.x = 0;
        if (current_mouse_state.y < 0) current_mouse_state.y = 0;
        if (current_mouse_state.x > 1024) current_mouse_state.x = 1024;
        if (current_mouse_state.y > 768) current_mouse_state.y = 768;

        /* Update button states */
        current_mouse_state.left_button = (mouse_bytes[0] & 0x01) != 0;
        current_mouse_state.right_button = (mouse_bytes[0] & 0x02) != 0;
        current_mouse_state.middle_button = (mouse_bytes[0] & 0x04) != 0;
    }
}

/* Get mouse state */
void mouse_get_state(struct mouse_state *state) {
    state->x = current_mouse_state.x;
    state->y = current_mouse_state.y;
    state->left_button = current_mouse_state.left_button;
    state->right_button = current_mouse_state.right_button;
    state->middle_button = current_mouse_state.middle_button;
}
