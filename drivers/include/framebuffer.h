#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>

/* Framebuffer functions */
void fb_init(void *addr, uint32_t width, uint32_t height, uint32_t pitch, uint16_t bpp);
void fb_clear(uint32_t color);
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color);
void fb_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t color);
void fb_swap(void);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);

/* Color helpers */
#define RGB(r, g, b) ((uint32_t)(((r) << 16) | ((g) << 8) | (b)))

#define COLOR_BLACK     RGB(0, 0, 0)
#define COLOR_WHITE     RGB(255, 255, 255)
#define COLOR_RED       RGB(255, 0, 0)
#define COLOR_GREEN     RGB(0, 255, 0)
#define COLOR_BLUE      RGB(0, 0, 255)
#define COLOR_YELLOW    RGB(255, 255, 0)
#define COLOR_CYAN      RGB(0, 255, 255)
#define COLOR_MAGENTA   RGB(255, 0, 255)
#define COLOR_GRAY      RGB(128, 128, 128)
#define COLOR_DARK_GRAY RGB(64, 64, 64)

#endif /* FRAMEBUFFER_H */
