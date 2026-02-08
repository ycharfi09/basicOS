#include "process.h"
#include "memory.h"
#include <stdint.h>
#include <stdbool.h>

/* Process management state */
static process_t *current_process = NULL;
static process_t *process_queue_head = NULL;
static process_t *process_queue_tail = NULL;
static uint32_t next_pid = 1;
static uint64_t system_ticks = 0;

/* Default time slice in ticks (10ms at 1000Hz) */
#define DEFAULT_TIME_SLICE 10

/* String copy helper */
static void strncpy_safe(char *dest, const char *src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/* Initialize process management */
void process_init(void) {
    current_process = NULL;
    process_queue_head = NULL;
    process_queue_tail = NULL;
    next_pid = 1;
    system_ticks = 0;
}

/* Create a new process */
process_t *process_create(const char *name, void (*entry_point)(void)) {
    process_t *proc = (process_t *)kmalloc(sizeof(process_t));
    if (!proc) return NULL;
    
    /* Initialize process structure */
    proc->pid = next_pid++;
    strncpy_safe(proc->name, name, 64);
    proc->state = PROCESS_READY;
    proc->priority = 1;
    proc->time_slice = DEFAULT_TIME_SLICE;
    proc->sleep_until = 0;
    proc->next = NULL;
    
    /* Allocate kernel stack (8KB) */
    proc->kernel_stack = (uint64_t)kmalloc(8192);
    if (!proc->kernel_stack) {
        kfree(proc);
        return NULL;
    }
    proc->kernel_stack += 8192;  /* Stack grows down */
    
    /* Create address space */
    proc->page_table = vmm_create_address_space();
    if (!proc->page_table) {
        kfree((void *)(proc->kernel_stack - 8192));
        kfree(proc);
        return NULL;
    }
    
    /* Initialize CPU context */
    memset(&proc->context, 0, sizeof(cpu_context_t));
    proc->context.rip = (uint64_t)entry_point;
    proc->context.rsp = proc->kernel_stack;
    proc->context.rflags = 0x202;  /* Interrupts enabled */
    proc->context.cr3 = (uint64_t)proc->page_table;
    
    return proc;
}

/* Destroy a process */
void process_destroy(process_t *proc) {
    if (!proc) return;
    
    /* Free page table */
    if (proc->page_table) {
        vmm_destroy_address_space(proc->page_table);
    }
    
    /* Free kernel stack */
    if (proc->kernel_stack) {
        kfree((void *)(proc->kernel_stack - 8192));
    }
    
    /* Free process structure */
    kfree(proc);
}

/* Get current running process */
process_t *process_get_current(void) {
    return current_process;
}

/* Yield CPU to another process */
void process_yield(void) {
    /* This will be called by scheduler_tick() */
}

/* Sleep for specified ticks */
void process_sleep(uint64_t ticks) {
    if (current_process) {
        current_process->sleep_until = system_ticks + ticks;
        current_process->state = PROCESS_BLOCKED;
    }
}

/* Exit current process */
void process_exit(int status) {
    (void)status;  /* TODO: Store exit status */
    if (current_process) {
        current_process->state = PROCESS_TERMINATED;
    }
}

/* Initialize scheduler */
void scheduler_init(void) {
    process_queue_head = NULL;
    process_queue_tail = NULL;
}

/* Add process to scheduler queue */
void scheduler_add(process_t *proc) {
    if (!proc) return;
    
    proc->next = NULL;
    if (!process_queue_head) {
        process_queue_head = proc;
        process_queue_tail = proc;
    } else {
        process_queue_tail->next = proc;
        process_queue_tail = proc;
    }
}

/* Remove process from scheduler queue */
void scheduler_remove(process_t *proc) {
    if (!proc || !process_queue_head) return;
    
    if (process_queue_head == proc) {
        process_queue_head = proc->next;
        if (!process_queue_head) {
            process_queue_tail = NULL;
        }
        return;
    }
    
    process_t *current = process_queue_head;
    while (current->next) {
        if (current->next == proc) {
            current->next = proc->next;
            if (process_queue_tail == proc) {
                process_queue_tail = current;
            }
            return;
        }
        current = current->next;
    }
}

/* Scheduler tick - called on timer interrupt */
void scheduler_tick(void) {
    system_ticks++;
    
    /* Wake up sleeping processes */
    process_t *proc = process_queue_head;
    while (proc) {
        if (proc->state == PROCESS_BLOCKED && proc->sleep_until > 0) {
            if (system_ticks >= proc->sleep_until) {
                proc->sleep_until = 0;
                proc->state = PROCESS_READY;
            }
        }
        proc = proc->next;
    }
    
    /* Decrement current process time slice */
    if (current_process && current_process->state == PROCESS_RUNNING) {
        if (current_process->time_slice > 0) {
            current_process->time_slice--;
        }
        
        /* If time slice expired, mark as ready for rescheduling */
        if (current_process->time_slice == 0) {
            current_process->state = PROCESS_READY;
            current_process->time_slice = DEFAULT_TIME_SLICE;
        }
    }
}

/* Get next process to run (simple round-robin) */
process_t *scheduler_next(void) {
    if (!process_queue_head) return NULL;
    
    /* Start from current process or head */
    process_t *start = current_process ? current_process->next : process_queue_head;
    if (!start) start = process_queue_head;
    
    process_t *proc = start;
    do {
        if (proc->state == PROCESS_READY) {
            proc->state = PROCESS_RUNNING;
            proc->time_slice = DEFAULT_TIME_SLICE;
            current_process = proc;
            return proc;
        }
        
        proc = proc->next;
        if (!proc) proc = process_queue_head;
    } while (proc != start);
    
    /* No ready process, keep current */
    return current_process;
}
