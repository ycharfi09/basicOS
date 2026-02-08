#include "keyboard.h"
#include <stdint.h>
#include <stdbool.h>

/* Keyboard ports */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Keyboard buffer */
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int keyboard_buffer_head = 0;
static int keyboard_buffer_tail = 0;

/* I/O port operations */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Scancode to ASCII mapping (US layout, simplified) */
static const char scancode_to_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

/* Initialize keyboard */
void keyboard_init(void) {
    keyboard_buffer_head = 0;
    keyboard_buffer_tail = 0;
}

/* Keyboard interrupt handler (called from IRQ1) */
void keyboard_interrupt_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    /* Ignore key release events (bit 7 set) */
    if (scancode & 0x80) {
        return;
    }

    /* Convert scancode to ASCII */
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c != 0) {
            /* Add to buffer */
            int next_head = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
            if (next_head != keyboard_buffer_tail) {
                keyboard_buffer[keyboard_buffer_head] = c;
                keyboard_buffer_head = next_head;
            }
        }
    }
}

/* Check if key is available */
bool keyboard_has_key(void) {
    return keyboard_buffer_head != keyboard_buffer_tail;
}

/* Get key from buffer */
char keyboard_get_key(void) {
    if (!keyboard_has_key()) {
        return 0;
    }

    char c = keyboard_buffer[keyboard_buffer_tail];
    keyboard_buffer_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}
