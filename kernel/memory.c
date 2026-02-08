#include "memory.h"
#include <stdint.h>
#include <stddef.h>

/* Simple heap allocator */
#define HEAP_SIZE (1024 * 1024 * 16)  /* 16 MB heap */

static uint8_t heap[HEAP_SIZE];
static size_t heap_offset = 0;

/* Initialize memory management */
void memory_init(void) {
    heap_offset = 0;
}

/* Simple bump allocator (no free support yet) */
void *kmalloc(size_t size) {
    /* Align to 16 bytes */
    size = (size + 15) & ~15;

    if (heap_offset + size > HEAP_SIZE) {
        return NULL;
    }

    void *ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

/* Free memory (not implemented in simple allocator) */
void kfree(void *ptr) {
    (void)ptr;
    /* TODO: Implement proper free */
}

/* Memory operations */
void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}
