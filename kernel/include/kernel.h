#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Kernel types */
typedef uint64_t uintptr_t;
typedef int64_t intptr_t;

/* Kernel panic */
void kernel_panic(const char *message);

/* Print functions */
void kprintf(const char *format, ...);

#endif /* KERNEL_H */
