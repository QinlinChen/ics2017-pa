#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

bool insert_wp(char *expr);
bool delete_wp(int N);
void print_all_wp();

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
	char expr[64];
	uint32_t pre_val;

} WP;

#endif
