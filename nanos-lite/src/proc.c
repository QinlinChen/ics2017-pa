#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;
PCB *current_game = &pcb[0];

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  // _switch(&pcb[i].as);
  // current = &pcb[i];
  // ((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

void switch_game() {
  current_game = (current_game == &pcb[0] ? &pcb[2] : &pcb[0]);
}

_RegSet* schedule(_RegSet *prev) {
  // printf("Hello from schedule\n");
  if (current)
    current->tf = prev;
  static int count_game = 0;
  // time for game and hello is 100 : 1 
  if (count_game >= 100 && current != &pcb[1]) {
    current = &pcb[1];
    count_game = 0;
  }
  else {
    current = current_game;
    count_game++;
  }
  _switch(&current->as); 
  return current->tf;
}
