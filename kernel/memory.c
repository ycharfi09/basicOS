#include "memory.h"
#include <stdint.h>
#include <stddef.h>

/* Improved heap allocator with free support */
#define HEAP_SIZE (1024 * 1024 * 16)  /* 16 MB heap */
#define HEAP_MAGIC 0xDEADBEEF

/* Heap block header */
typedef struct heap_block {
    uint32_t magic;           /* Magic number for validation */
    size_t size;              /* Size of the block (excluding header) */
    struct heap_block *next;  /* Next free block */
    bool free;                /* Is this block free? */
} heap_block_t;

static uint8_t heap[HEAP_SIZE];
static heap_block_t *heap_start = NULL;
static heap_block_t *free_list = NULL;

/* Initialize memory management */
void memory_init(void) {
    /* Initialize the heap with one large free block */
    heap_start = (heap_block_t *)heap;
    heap_start->magic = HEAP_MAGIC;
    heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_start->next = NULL;
    heap_start->free = true;
    free_list = heap_start;
}

/* Find a free block that fits the requested size (first fit) */
static heap_block_t *find_free_block(size_t size) {
    heap_block_t *current = free_list;
    while (current) {
        if (current->free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/* Split a block if it's large enough */
static void split_block(heap_block_t *block, size_t size) {
    if (block->size >= size + sizeof(heap_block_t) + 16) {
        heap_block_t *new_block = (heap_block_t *)((uint8_t *)block + sizeof(heap_block_t) + size);
        new_block->magic = HEAP_MAGIC;
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->free = true;
        new_block->next = block->next;
        
        block->size = size;
        block->next = new_block;
    }
}

/* Allocate memory from heap */
void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    /* Align to 16 bytes */
    size = (size + 15) & ~15;
    
    /* Find a free block */
    heap_block_t *block = find_free_block(size);
    if (!block) {
        return NULL;  /* Out of memory */
    }
    
    /* Split the block if possible */
    split_block(block, size);
    
    /* Mark as allocated */
    block->free = false;
    
    /* Return pointer after the header */
    return (void *)((uint8_t *)block + sizeof(heap_block_t));
}

/* Merge adjacent free blocks */
static void merge_free_blocks(void) {
    heap_block_t *current = free_list;
    while (current && current->next) {
        if (current->free && current->next->free) {
            /* Merge with next block */
            uint8_t *current_end = (uint8_t *)current + sizeof(heap_block_t) + current->size;
            if (current_end == (uint8_t *)current->next) {
                current->size += sizeof(heap_block_t) + current->next->size;
                current->next = current->next->next;
                continue;  /* Check again with same block */
            }
        }
        current = current->next;
    }
}

/* Free allocated memory */
void kfree(void *ptr) {
    if (!ptr) return;
    
    /* Get the block header */
    heap_block_t *block = (heap_block_t *)((uint8_t *)ptr - sizeof(heap_block_t));
    
    /* Validate magic number */
    if (block->magic != HEAP_MAGIC) {
        return;  /* Invalid pointer or corrupted memory */
    }
    
    /* Mark as free */
    block->free = true;
    
    /* Merge adjacent free blocks */
    merge_free_blocks();
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
