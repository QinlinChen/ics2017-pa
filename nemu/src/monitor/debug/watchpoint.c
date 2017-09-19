#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "cpu/reg.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

static WP *new_wp() {
	Assert(free_, "No extra space in wp_pool!");
	WP *ret = free_;
	free_ = free_->next;
	return ret;
}

static void free_wp(WP *wp) {
	wp->next = free_;
	free_ = wp;
}

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

bool insert_wp(char *expression) {
	if (strlen(expression) >= 64) 
		return false;
	
	bool success;
	uint32_t val = expr(expression, &success);
	if (!success)
		return false;

	WP *wp = new_wp();
	strcpy(wp->expr, expression);
	wp->pre_val = val;
	wp->next = head;
	head = wp;
	return true;
}

bool delete_wp(int N) {
	WP *pre = NULL, *cur = head;
	while (cur != NULL) {
		if (cur->NO == N) {
			cur->expr[0] = '\0';
			if (!pre) 
				head = cur->next;
			else	
				pre->next = cur->next;
			free_wp(cur);
			return true;	
		}
		pre = cur;
		cur = cur->next;
	}
	return false;
}

void print_all_wp() {
	if (!head) {
		printf("No watchpoints!\n");
		return;
	}
	printf("Num\tWhat\n");
	WP *p;
	for (p = head; p != NULL; p = p->next) 
		printf("%d\t%s\n", p->NO, p->expr);
}

bool check_watchpoints() {
	WP *p;
	bool success;
	bool hit_watchpoint = false;
	uint32_t new_val, pre_val;

	for (p = head; p != NULL; p = p->next) {
		new_val = expr(p->expr, &success);
		pre_val = p->pre_val;
		if (!success)
			return false;
		if (new_val != pre_val) {
			printf("Hit watchpoint %d: %s\n", p->NO, p->expr);
			printf("at 0x%x\n", cpu.eip);
			printf("Old value = %uU = %d = 0x%x\n",
					   pre_val, (int)pre_val, pre_val);
			printf("New value = %uU = %d = 0x%x\n",
					   new_val, (int)new_val, new_val);
			p->pre_val = new_val;
			hit_watchpoint = true;
		}
	}
	return hit_watchpoint;
}

