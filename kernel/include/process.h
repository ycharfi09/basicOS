#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "paging.h"

/* Process states */
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

/* CPU context for context switching */
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t rflags;
    uint64_t cr3;  /* Page table base */
} cpu_context_t;

/* Process Control Block (PCB) */
typedef struct process {
    uint32_t pid;                    /* Process ID */
    char name[64];                   /* Process name */
    process_state_t state;           /* Current state */
    cpu_context_t context;           /* Saved CPU context */
    pml4_t *page_table;             /* Virtual memory space */
    uint64_t kernel_stack;           /* Kernel stack pointer */
    uint32_t priority;               /* Scheduling priority */
    uint64_t time_slice;             /* Time slice in ticks */
    uint64_t sleep_until;            /* Wake up time (0 = not sleeping) */
    struct process *next;            /* Next process in queue */
} process_t;

/* Process management functions */
void process_init(void);
process_t *process_create(const char *name, void (*entry_point)(void));
void process_destroy(process_t *proc);
process_t *process_get_current(void);
void process_yield(void);
void process_sleep(uint64_t ticks);
void process_exit(int status);

/* Scheduler functions */
void scheduler_init(void);
void scheduler_add(process_t *proc);
void scheduler_remove(process_t *proc);
void scheduler_tick(void);
process_t *scheduler_next(void);

/* Context switching */
extern void context_switch(cpu_context_t *old_context, cpu_context_t *new_context);

#endif /* PROCESS_H */
