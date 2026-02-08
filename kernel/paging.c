#include "paging.h"
#include "memory.h"
#include <stdint.h>
#include <stdbool.h>

/* Physical memory manager state */
static uint8_t *pmm_bitmap = NULL;
static size_t pmm_bitmap_size = 0;
static size_t pmm_total_frames = 0;
static size_t pmm_free_frames = 0;

/* Kernel page tables */
static pml4_t *kernel_pml4 = NULL;
static pml4_t *current_pml4 = NULL;

/* Bitmap operations */
static inline void bitmap_set(uint8_t *bitmap, size_t bit) {
    bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void bitmap_clear(uint8_t *bitmap, size_t bit) {
    bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline bool bitmap_test(uint8_t *bitmap, size_t bit) {
    return bitmap[bit / 8] & (1 << (bit % 8));
}

/* Physical memory manager initialization */
void pmm_init(void *bitmap, size_t memory_size) {
    pmm_bitmap = (uint8_t *)bitmap;
    pmm_total_frames = memory_size / PAGE_SIZE;
    pmm_bitmap_size = (pmm_total_frames + 7) / 8;
    
    /* Mark all frames as used initially */
    memset(pmm_bitmap, 0xFF, pmm_bitmap_size);
    pmm_free_frames = 0;
}

/* Allocate a physical frame */
void *pmm_alloc_frame(void) {
    for (size_t i = 0; i < pmm_total_frames; i++) {
        if (!bitmap_test(pmm_bitmap, i)) {
            bitmap_set(pmm_bitmap, i);
            pmm_free_frames--;
            return (void *)(i * PAGE_SIZE);
        }
    }
    return NULL;  /* Out of memory */
}

/* Free a physical frame */
void pmm_free_frame(void *frame) {
    size_t frame_index = (size_t)frame / PAGE_SIZE;
    if (frame_index < pmm_total_frames && bitmap_test(pmm_bitmap, frame_index)) {
        bitmap_clear(pmm_bitmap, frame_index);
        pmm_free_frames++;
    }
}

/* Get free memory in bytes */
size_t pmm_get_free_memory(void) {
    return pmm_free_frames * PAGE_SIZE;
}

/* Get used memory in bytes */
size_t pmm_get_used_memory(void) {
    return (pmm_total_frames - pmm_free_frames) * PAGE_SIZE;
}

/* Helper to get page table indices */
static inline uint64_t pml4_index(uint64_t virt) {
    return (virt >> 39) & 0x1FF;
}

static inline uint64_t pdp_index(uint64_t virt) {
    return (virt >> 30) & 0x1FF;
}

static inline uint64_t pd_index(uint64_t virt) {
    return (virt >> 21) & 0x1FF;
}

static inline uint64_t pt_index(uint64_t virt) {
    return (virt >> 12) & 0x1FF;
}

/* Create a new address space */
pml4_t *vmm_create_address_space(void) {
    pml4_t *pml4 = (pml4_t *)pmm_alloc_frame();
    if (!pml4) return NULL;
    
    memset(pml4, 0, sizeof(pml4_t));
    return pml4;
}

/* Destroy an address space */
void vmm_destroy_address_space(pml4_t *pml4) {
    if (!pml4) return;
    
    /* Free all page tables (simplified - would need to recursively free) */
    pmm_free_frame(pml4);
}

/* Map a virtual page to a physical frame */
bool vmm_map_page(pml4_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    if (!pml4) return false;
    
    /* Get indices */
    uint64_t pml4i = pml4_index(virt);
    uint64_t pdpi = pdp_index(virt);
    uint64_t pdi = pd_index(virt);
    uint64_t pti = pt_index(virt);
    
    /* Get or create PDP */
    page_directory_pointer_t *pdp;
    if (!(pml4->entries[pml4i] & PAGE_PRESENT)) {
        pdp = (page_directory_pointer_t *)pmm_alloc_frame();
        if (!pdp) return false;
        memset(pdp, 0, sizeof(page_directory_pointer_t));
        pml4->entries[pml4i] = (uint64_t)pdp | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
    } else {
        pdp = (page_directory_pointer_t *)(pml4->entries[pml4i] & ~0xFFF);
    }
    
    /* Get or create PD */
    page_directory_t *pd;
    if (!(pdp->entries[pdpi] & PAGE_PRESENT)) {
        pd = (page_directory_t *)pmm_alloc_frame();
        if (!pd) return false;
        memset(pd, 0, sizeof(page_directory_t));
        pdp->entries[pdpi] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
    } else {
        pd = (page_directory_t *)(pdp->entries[pdpi] & ~0xFFF);
    }
    
    /* Get or create PT */
    page_table_t *pt;
    if (!(pd->entries[pdi] & PAGE_PRESENT)) {
        pt = (page_table_t *)pmm_alloc_frame();
        if (!pt) return false;
        memset(pt, 0, sizeof(page_table_t));
        pd->entries[pdi] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
    } else {
        pt = (page_table_t *)(pd->entries[pdi] & ~0xFFF);
    }
    
    /* Map the page */
    pt->entries[pti] = phys | flags;
    
    /* Flush TLB for this page */
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
    
    return true;
}

/* Unmap a virtual page */
void vmm_unmap_page(pml4_t *pml4, uint64_t virt) {
    if (!pml4) return;
    
    uint64_t pml4i = pml4_index(virt);
    if (!(pml4->entries[pml4i] & PAGE_PRESENT)) return;
    
    page_directory_pointer_t *pdp = (page_directory_pointer_t *)(pml4->entries[pml4i] & ~0xFFF);
    uint64_t pdpi = pdp_index(virt);
    if (!(pdp->entries[pdpi] & PAGE_PRESENT)) return;
    
    page_directory_t *pd = (page_directory_t *)(pdp->entries[pdpi] & ~0xFFF);
    uint64_t pdi = pd_index(virt);
    if (!(pd->entries[pdi] & PAGE_PRESENT)) return;
    
    page_table_t *pt = (page_table_t *)(pd->entries[pdi] & ~0xFFF);
    uint64_t pti = pt_index(virt);
    
    /* Clear the entry */
    pt->entries[pti] = 0;
    
    /* Flush TLB */
    __asm__ volatile ("invlpg (%0)" :: "r"(virt) : "memory");
}

/* Get physical address for virtual address */
uint64_t vmm_get_physical(pml4_t *pml4, uint64_t virt) {
    if (!pml4) return 0;
    
    uint64_t pml4i = pml4_index(virt);
    if (!(pml4->entries[pml4i] & PAGE_PRESENT)) return 0;
    
    page_directory_pointer_t *pdp = (page_directory_pointer_t *)(pml4->entries[pml4i] & ~0xFFF);
    uint64_t pdpi = pdp_index(virt);
    if (!(pdp->entries[pdpi] & PAGE_PRESENT)) return 0;
    
    page_directory_t *pd = (page_directory_t *)(pdp->entries[pdpi] & ~0xFFF);
    uint64_t pdi = pd_index(virt);
    if (!(pd->entries[pdi] & PAGE_PRESENT)) return 0;
    
    page_table_t *pt = (page_table_t *)(pd->entries[pdi] & ~0xFFF);
    uint64_t pti = pt_index(virt);
    if (!(pt->entries[pti] & PAGE_PRESENT)) return 0;
    
    return (pt->entries[pti] & ~0xFFF) | (virt & 0xFFF);
}

/* Switch to a different address space */
void vmm_switch_address_space(pml4_t *pml4) {
    if (!pml4) return;
    
    current_pml4 = pml4;
    __asm__ volatile ("mov %0, %%cr3" :: "r"((uint64_t)pml4) : "memory");
}

/* Initialize virtual memory manager */
void vmm_init(void) {
    /* Create kernel address space */
    kernel_pml4 = vmm_create_address_space();
    current_pml4 = kernel_pml4;
    
    /* Identity map the first 16MB for kernel */
    for (uint64_t i = 0; i < 0x1000000; i += PAGE_SIZE) {
        vmm_map_page(kernel_pml4, i, i, PAGE_PRESENT | PAGE_WRITE);
    }
    
    /* Switch to the kernel address space */
    vmm_switch_address_space(kernel_pml4);
}

/* Initialize paging subsystem */
void paging_init(void) {
    /* Note: PMM must be initialized separately with proper memory map from bootloader */
    vmm_init();
}
