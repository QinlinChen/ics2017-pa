#include "common.h"
#include "syscall.h"

_RegSet* sys_none(_RegSet *r) {
  SYSCALL_ARG1(r) = 1;
  return NULL;
}

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);

  switch (a[0]) {
    case SYS_none: return sys_none(r);
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}
