#include "cpu/exec.h"

make_EHelper(test) {
  rtl_and(&t0, &id_dest->val, &id_src->val);
  
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t0, id_dest->width);

  print_asm_template2(test);
}

make_EHelper(and) {
  rtl_and(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);

  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t0, id_dest->width);
  
  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t0, id_dest->width);
  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);
  rtl_update_ZFSF(&t0, id_dest->width);

  print_asm_template2(or);
}

make_EHelper(sar) {
  rtl_sext(&t0, &id_dest->val, id_dest->width);
  rtl_sar(&t0, &t0, &id_src->val);
  operand_write(id_dest, &t0);
  // unnecessary to update CF and OF in NEMU
  rtl_update_ZFSF(&t0, id_dest->width);
  
  print_asm_template2(sar);
}

make_EHelper(shl) {
  rtl_shl(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  // unnecessary to update CF and OF in NEMU
  rtl_update_ZFSF(&t0, id_dest->width);
  
  print_asm_template2(shl);
}

make_EHelper(shr) {
  rtl_shr(&t0, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t0);
  // unnecessary to update CF and OF in NEMU
  rtl_update_ZFSF(&t0, id_dest->width);
  
  print_asm_template2(shr);
}

make_EHelper(rol) {
  int i;
  rtl_mv(&t1, &id_dest->val);
  for (i = 0; i < id_src->val; ++i) {
    rtl_msb(&t0, &t1, id_dest->width);
    rtl_shli(&t1, &t1, 1);
    rtl_or(&t1, &t1, &t0);
  }
  operand_write(id_dest, &t1);
  rtl_set_CF(&t0);
  rtl_msb(&t1, &t1, id_dest->width);
  rtl_xor(&t0, &t1, &t0);
  rtl_set_OF(&t0);
  
  print_asm_template2(rol);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  rtl_mv(&t0, &id_dest->val);
  rtl_not(&t0);
  operand_write(id_dest, &t0);

  print_asm_template1(not);
}
