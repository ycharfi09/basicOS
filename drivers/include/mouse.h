#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

/* Mouse state */
struct mouse_state {
    int32_t x;
    int32_t y;
    bool left_button;
    bool right_button;
    bool middle_button;
};

/* Mouse functions */
void mouse_init(void);
void mouse_get_state(struct mouse_state *state);

#endif /* MOUSE_H */
