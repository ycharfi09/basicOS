#include "kernel.h"
#include "limine.h"
#include "memory.h"
#include "gdt.h"
#include "idt.h"
#include "../drivers/include/framebuffer.h"
#include "../drivers/include/pic.h"
#include "../drivers/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../drivers/include/mouse.h"
#include <stdint.h>

/* Limine requests */
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = {LIMINE_COMMON_MAGIC, 0x9d5827dcd881dd75, 0xa3148604f6fab11b},
    .revision = 0
};

static volatile struct limine_memmap_request memmap_request = {
    .id = {LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62},
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = {LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b},
    .revision = 0
};

/* Global framebuffer pointer */
struct limine_framebuffer *fb = NULL;

/* Halt the CPU */
static void halt(void) {
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

/* Kernel panic */
void kernel_panic(const char *message) {
    (void)message;
    // TODO: Display error message
    halt();
}

/* Main kernel entry point */
void kernel_main(void) {
    /* Initialize GDT */
    gdt_init();

    /* Initialize IDT */
    idt_init();

    /* Get framebuffer from Limine */
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        halt();
    }

    fb = framebuffer_request.response->framebuffers[0];

    /* Initialize memory management */
    memory_init();

    /* Initialize framebuffer driver */
    fb_init(fb->address, fb->width, fb->height, fb->pitch, fb->bpp);

    /* Initialize PIC */
    pic_init();

    /* Initialize timer (1000 Hz = 1ms per tick) */
    timer_init(1000);

    /* Initialize keyboard */
    keyboard_init();

    /* Initialize mouse */
    mouse_init();

    /* Enable interrupts */
    __asm__ volatile ("sti");

    /* Clear screen to dark blue */
    fb_clear(RGB(20, 30, 50));

    /* Draw welcome message */
    fb_draw_string(10, 10, "BasicOS v1.0", COLOR_WHITE);
    fb_draw_string(10, 30, "Booting...", COLOR_WHITE);

    /* Wait a bit for effect */
    timer_wait(1000);

    /* Initialize GUI */
    extern void gui_init(void);
    extern void gui_update(void);
    extern void gui_render(void);
    gui_init();

    /* Main loop */
    while (1) {
        gui_update();
        gui_render();
        timer_wait(16);  /* ~60 FPS */
    }
}
