#ifndef MINIBWA_WINDOWS_SYS_MMAN_H
#define MINIBWA_WINDOWS_SYS_MMAN_H

#include <stddef.h>
#include <sys/types.h>

#define PROT_READ 0x1
#define MAP_SHARED 0x1
#define MAP_FAILED ((void *)-1)

void *minibwa_mmap(void *addr, size_t length, int prot, int flags, int fd,
                   off_t offset);
int minibwa_munmap(void *addr, size_t length);

#define mmap minibwa_mmap
#define munmap minibwa_munmap

#endif
