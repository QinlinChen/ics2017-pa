#include "nemu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

int is_mmio(paddr_t addr);
uint32_t mmio_read(paddr_t addr, int len, int map_NO);
void mmio_write(paddr_t addr, int len, uint32_t data, int map_NO);
/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO;
  if ((map_NO = is_mmio(addr)) < 0)
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  return mmio_read(addr, len, map_NO);
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO;
  if ((map_NO = is_mmio(addr)) < 0)
    memcpy(guest_to_host(addr), &data, len);
  else
    mmio_write(addr, len, data, map_NO);
}

static paddr_t page_translate(vaddr_t addr) {
  return addr;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if ((addr & ~0xfff) != ((addr + len) & ~0xfff))
    assert(0);
  else
    return paddr_read(page_translate(addr), len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if ((addr & ~0xfff) != ((addr + len) & ~0xfff))
    assert(0);
  else
    return paddr_write(page_translate(addr), len, data);
}
