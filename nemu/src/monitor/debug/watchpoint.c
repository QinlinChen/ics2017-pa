#include "monitor/watchpoint.h"
#include "monitor/expr.h"

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

bool insert_wp(char *expr) {
	WP *wp = new_wp();
	if (strlen(expr) >= 64) 
		return false;
	strcpy(wp->expr, expr);
	
	wp->next = head;
	head = wp->next;
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
	WP *p;
	printf("Num\tWhat\n");
	for (p = head; p != NULL; ++p) 
		printf("%d\t%s\n", p->NO, p->expr);
}



