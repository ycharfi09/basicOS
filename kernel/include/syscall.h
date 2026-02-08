#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

/* System call numbers */
#define SYS_EXIT        0
#define SYS_FORK        1
#define SYS_READ        2
#define SYS_WRITE       3
#define SYS_OPEN        4
#define SYS_CLOSE       5
#define SYS_WAIT        6
#define SYS_EXEC        7
#define SYS_GETPID      8
#define SYS_SLEEP       9
#define SYS_YIELD       10

/* System call handler */
void syscall_init(void);
uint64_t syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

#endif /* SYSCALL_H */
