#include "syscall.h"
#include "process.h"
#include "vfs.h"
#include <stdint.h>

/* System call implementations */

static uint64_t sys_exit(uint64_t status) {
    process_exit((int)status);
    return 0;
}

static uint64_t sys_fork(void) {
    /* TODO: Implement fork */
    return 0;
}

static uint64_t sys_read(uint64_t fd, uint64_t buffer, uint64_t size) {
    return (uint64_t)vfs_read((int)fd, (void *)buffer, (uint32_t)size);
}

static uint64_t sys_write(uint64_t fd, uint64_t buffer, uint64_t size) {
    return (uint64_t)vfs_write((int)fd, (const void *)buffer, (uint32_t)size);
}

static uint64_t sys_open(uint64_t path) {
    return (uint64_t)vfs_open((const char *)path);
}

static uint64_t sys_close(uint64_t fd) {
    vfs_close((int)fd);
    return 0;
}

static uint64_t sys_wait(void) {
    /* TODO: Implement wait */
    return 0;
}

static uint64_t sys_exec(void) {
    /* TODO: Implement exec */
    return 0;
}

static uint64_t sys_getpid(void) {
    process_t *current = process_get_current();
    return current ? current->pid : 0;
}

static uint64_t sys_sleep(uint64_t ticks) {
    process_sleep(ticks);
    return 0;
}

static uint64_t sys_yield(void) {
    process_yield();
    return 0;
}

/* System call handler */
uint64_t syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (syscall_num) {
        case SYS_EXIT:
            return sys_exit(arg1);
        case SYS_FORK:
            return sys_fork();
        case SYS_READ:
            return sys_read(arg1, arg2, arg3);
        case SYS_WRITE:
            return sys_write(arg1, arg2, arg3);
        case SYS_OPEN:
            return sys_open(arg1);
        case SYS_CLOSE:
            return sys_close(arg1);
        case SYS_WAIT:
            return sys_wait();
        case SYS_EXEC:
            return sys_exec();
        case SYS_GETPID:
            return sys_getpid();
        case SYS_SLEEP:
            return sys_sleep(arg1);
        case SYS_YIELD:
            return sys_yield();
        default:
            return (uint64_t)-1;
    }
}

/* Initialize system call interface */
void syscall_init(void) {
    /* System calls will be invoked through software interrupts */
    /* For now, we'll use a simple function call interface */
}
