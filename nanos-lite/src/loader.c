#include "common.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

void* new_page(void);

int fs_open(const char *pathname, int flags, int mode);
size_t fs_filesz(int fd);
ssize_t fs_read(int fd, void *buf, size_t len);
int fs_close(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  int fd = fs_open(filename, 0, 0);
  int i, pages = fs_filesz(fd) / PGSIZE + 1;
  void *pa, *va;
  
  for (i = 0, va = DEFAULT_ENTRY; i < pages; ++i, va += PGSIZE) {
    pa = new_page();
    // printf("pa: %x, va: %x", pa, va);
    // printf(", read: %d\n", fs_read(fd, pa, PGSIZE));
    fs_read(fd, pa, PGSIZE);
    _map(as, va, pa);
  }

  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
