#ifndef STDDEF_H
#define STDDEF_H

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned long long size_t;
typedef long long ssize_t;
#endif

#define NULL ((void *)0)

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif /* STDDEF_H */
