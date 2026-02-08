#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

/* Keyboard functions */
void keyboard_init(void);
bool keyboard_has_key(void);
char keyboard_get_key(void);

#endif /* KEYBOARD_H */
