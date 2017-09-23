#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

bool insert_wp(char *);
bool delete_wp(int);
void print_all_wp();
bool check_watchpoints();

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
	char expr[64];
	uint32_t pre_val;

} WP;

#endif
