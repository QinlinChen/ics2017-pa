#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  if (NO > cpu.idtr.limit)
    assert(0);
  rtl_push(&cpu.eflags);
  cpu.IF = 0;
  rtl_push(&cpu.cs);
  rtl_push(&ret_addr);
  
  vaddr_t idt_addr = cpu.idtr.base + NO * 8;
  uint32_t low = vaddr_read(idt_addr, 4);
  uint32_t high = vaddr_read(idt_addr + 4, 4);
  vaddr_t jmp_addr = ((low & 0x0000ffff) | (high & 0xffff0000));
  decoding.jmp_eip = jmp_addr;
  decoding.is_jmp = 1;
}

void dev_raise_intr() {
  cpu.INTR = true;
}
