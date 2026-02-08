#include "kernel.h"
#include "limine.h"
#include "memory.h"
#include "gdt.h"
#include "idt.h"
#include "log.h"
#include "process.h"
#include "syscall.h"
#include "vfs.h"
#include "../drivers/include/framebuffer.h"
#include "../drivers/include/pic.h"
#include "../drivers/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../drivers/include/mouse.h"
#include "../drivers/include/ata.h"
#include <stdint.h>

/* Limine requests - marked as used to prevent compiler optimization */
__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = {LIMINE_COMMON_MAGIC, 0x9d5827dcd881dd75, 0xa3148604f6fab11b},
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = {LIMINE_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62},
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = {LIMINE_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b},
    .revision = 0
};

/* Limine base revision - required for protocol version checking */
__attribute__((used, section(".requests")))
static volatile uint64_t limine_base_revision[3] = {
    0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, 0
};

/* Global framebuffer pointer */
struct limine_framebuffer *fb = NULL;

/* Simple port I/O functions */
static inline void __outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t __inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Simple serial port output for debugging */
static void serial_write_string(const char *str) {
    const uint16_t COM1 = 0x3F8;
    while (*str) {
        while ((__inb(COM1 + 5) & 0x20) == 0);
        __outb(COM1, *str++);
    }
}

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
    serial_write_string("BasicOS: Kernel starting...\n");
    
    /* Initialize GDT */
    gdt_init();
    serial_write_string("BasicOS: GDT initialized\n");

    /* Initialize IDT */
    idt_init();
    serial_write_string("BasicOS: IDT initialized\n");

    /* Get framebuffer from Limine */
    serial_write_string("BasicOS: Checking framebuffer...\n");
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        serial_write_string("BasicOS: ERROR - No framebuffer from bootloader!\n");
        halt();
    }
    serial_write_string("BasicOS: Framebuffer OK\n");

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
    
    /* Initialize logging system */
    log_init();
    LOG_INFO_MSG("Kernel", "BasicOS v2.0 - Daily Driver Edition");
    
    /* Initialize ATA disk driver */
    ata_init();
    if (ata_drive_present()) {
        LOG_INFO_MSG("Storage", "ATA disk detected");
    } else {
        LOG_WARN_MSG("Storage", "No ATA disk found");
    }
    
    /* Initialize VFS */
    vfs_init();
    LOG_INFO_MSG("VFS", "Virtual File System initialized");
    
    /* Initialize process management */
    process_init();
    scheduler_init();
    LOG_INFO_MSG("Scheduler", "Process scheduler initialized");
    
    /* Initialize system calls */
    syscall_init();
    LOG_INFO_MSG("Syscall", "System call interface initialized");

    /* Enable interrupts */
    __asm__ volatile ("sti");

    /* Clear screen to dark blue */
    fb_clear(RGB(20, 30, 50));

    /* Draw welcome message */
    fb_draw_string(10, 10, "BasicOS v2.0 - Daily Driver Edition", COLOR_WHITE);
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
