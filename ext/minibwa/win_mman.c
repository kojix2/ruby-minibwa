#include <errno.h>
#include <stdint.h>
#include <windows.h>
#include <io.h>

#include "compat/sys/mman.h"

void *
minibwa_mmap(void *addr, size_t length, int prot, int flags, int fd,
             off_t offset)
{
    HANDLE file;
    HANDLE mapping;
    void *view;
    uint64_t map_offset;

    (void)addr;
    (void)prot;
    (void)flags;

    file = (HANDLE)_get_osfhandle(fd);
    if (file == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return MAP_FAILED;
    }

    mapping = CreateFileMappingA(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (mapping == NULL) {
        errno = EINVAL;
        return MAP_FAILED;
    }

    map_offset = (uint64_t)offset;
    view = MapViewOfFile(mapping, FILE_MAP_READ, (DWORD)(map_offset >> 32),
                         (DWORD)map_offset, length);
    CloseHandle(mapping);
    if (view == NULL) {
        errno = EINVAL;
        return MAP_FAILED;
    }

    return view;
}

int
minibwa_munmap(void *addr, size_t length)
{
    (void)length;
    if (UnmapViewOfFile(addr)) return 0;
    errno = EINVAL;
    return -1;
}
