#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Page sizes */
#define PAGE_SIZE 4096
#define PAGE_ENTRIES 512

/* Page flags */
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_WRITETHROUGH (1 << 3)
#define PAGE_NOCACHE    (1 << 4)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_HUGE       (1 << 7)
#define PAGE_GLOBAL     (1 << 8)
#define PAGE_NX         (1ULL << 63)

/* Page table entry */
typedef uint64_t pte_t;

/* Page directory pointer table entry */
typedef uint64_t pdpe_t;

/* Page directory entry */
typedef uint64_t pde_t;

/* Page map level 4 entry */
typedef uint64_t pml4e_t;

/* Page table structures */
typedef struct {
    pte_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

typedef struct {
    pde_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

typedef struct {
    pdpe_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_directory_pointer_t;

typedef struct {
    pml4e_t entries[PAGE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) pml4_t;

/* Physical memory frame allocator */
void pmm_init(void *bitmap, size_t memory_size);
void *pmm_alloc_frame(void);
void pmm_free_frame(void *frame);
size_t pmm_get_free_memory(void);
size_t pmm_get_used_memory(void);

/* Virtual memory manager */
void vmm_init(void);
pml4_t *vmm_create_address_space(void);
void vmm_destroy_address_space(pml4_t *pml4);
bool vmm_map_page(pml4_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap_page(pml4_t *pml4, uint64_t virt);
uint64_t vmm_get_physical(pml4_t *pml4, uint64_t virt);
void vmm_switch_address_space(pml4_t *pml4);

/* Paging initialization */
void paging_init(void);

#endif /* PAGING_H */
