#include "nemu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

// Page directory and page table constants
#define PGSHFT    12      // log2(PGSIZE)
#define PTXSHFT   12      // Offset of PTX in a linear address
#define PDXSHFT   22      // Offset of PDX in a linear address

// Page table/directory entry flags
#define PTE_P     0x001     // Present
#define PTE_A     0x020     // Accessed
#define PTE_D     0x040     // Dirty

#define PDX(va)     (((uint32_t)(va) >> PDXSHFT) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> PTXSHFT) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint32_t)(pte) & ~0xfff)

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
  if (!cpu.PG)
    return (paddr_t)addr;
    
  uint32_t PDE, PTE;
  PDE = paddr_read(cpu.cr3 + 4 * PDX(addr), 4);
  assert((PDE & PTE_P) == 1);
  
  PTE = paddr_read(PTE_ADDR(PDE) + 4 * PTX(addr), 4);
  assert((PTE & PTE_P) == 1);
  
  return (paddr_t)(PTE_ADDR(PTE) | OFF(addr));
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if ((addr & ~0xfff) != ((addr + len - 1) & ~0xfff)) {
    assert(0);
  }   
  else {
    paddr_t paddr = page_translate(addr);
    printf("vaddr: %x, paddr: %x", addr, paddr);
    return paddr_read(paddr, len);
  }
    
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if ((addr & ~0xfff) != ((addr + len - 1) & ~0xfff)) {
    assert(0);
  }  
  else
    return paddr_write(page_translate(addr), len, data);
}
