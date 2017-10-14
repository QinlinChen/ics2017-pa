#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

// 32bit only
make_EHelper(push) {
  rtl_push(&id_dest->val);

  print_asm_template1(push);
}

// 32bit only
make_EHelper(pop) {
  rtl_pop(&t0);
  operand_write(id_dest, &t0);

  print_asm_template1(pop);
}

make_EHelper(pusha) {
  TODO();

  print_asm("pusha");
}

make_EHelper(popa) {
  TODO();

  print_asm("popa");
}

make_EHelper(leave) {
  rtl_mv(&reg_l(R_ESP), &reg_l(R_EBP));
  rtl_pop(&reg_l(R_EBP));
  
  print_asm("leave");
}

make_EHelper(cltd) {
  if (decoding.is_operand_size_16) {
    rtl_sext(&t0, &reg_l(R_EAX), 2);
    rtl_shri(&reg_l(R_EDX), &t0, 16);
  }
  else {
    rtl_sari(&reg_l(R_EDX), &reg_l(R_EAX), 31);
  }

  print_asm(decoding.is_operand_size_16 ? "cwtl" : "cltd");
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    rtl_shli(&reg_l(R_EAX), &reg_l(R_EAX), 24);
    rtl_sari(&reg_l(R_EAX), &reg_l(R_EAX), 8);
    rtl_shri(&reg_l(R_EAX), &reg_l(R_EAX), 16);
  }
  else {
    rtl_sext(&reg_l(R_EAX), &reg_l(R_EAX), 2);
  }

  print_asm(decoding.is_operand_size_16 ? "cbtw" : "cwtl");
}

make_EHelper(movsx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  rtl_sext(&t2, &id_src->val, id_src->width);
  operand_write(id_dest, &t2);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  id_dest->width = decoding.is_operand_size_16 ? 2 : 4;
  operand_write(id_dest, &id_src->val);
  print_asm_template2(movzx);
}

make_EHelper(lea) {
  rtl_li(&t2, id_src->addr);
  operand_write(id_dest, &t2);
  print_asm_template2(lea);
}

make_EHelper(xchg) {
  rtl_li(&t0, id_src->val);
  operand_write(id_dest, &t0);
  rtl_li(&t0, id_dest->val);
  operand_write(id_src, &t0);
  print_asm_template2(xchg);
}
