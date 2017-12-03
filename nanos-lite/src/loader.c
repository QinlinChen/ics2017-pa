#include "common.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

int fs_open(const char *pathname, int flags, int mode);
size_t fs_filesz(int fd);
ssize_t fs_read(int fd, void *buf, size_t len);
int fs_close(int fd);
off_t fs_begin(int fd);
off_t fs_end(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  uintptr_t pa_begin, pa_end, pa;
  uintptr_t va;
  
  pa_begin = (uintptr_t)fs_begin(fd) & ~0xfff;
  pa_end = ((uintptr_t)fs_end(fd) - 1) & ~0xfff;
  for (pa = pa_begin, va = (uintptr_t)DEFAULT_ENTRY; pa <= pa_end; pa += PGSIZE, va += PGSIZE) {
    printf("va: %x, pa: %x\n", va, pa);
    _map(as, (void *)va, (void *)pa);
  }
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
