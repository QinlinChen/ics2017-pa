#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

void exec_wrapper(bool);

static int cmd_si(char *args) {
	char *arg = strtok(NULL, " ");
	int N = 0;
	if (!arg)
		N = 1;				//if arg is NULL, execute 1 instruction
	else if ((N = atoi(arg)) <= 0) {
		print_error("Argument Error: Argument should be a positive integer!");
		return 0;
	}

	cpu_exec(N);
	return 0;
}

static int cmd_info(char *args) {
	char *arg = strtok(NULL, " ");
	if (!arg) {
		print_error("Argument Error: There should be an argument!");
		return 0;
	}
	
	int i;	
	switch (arg[0]) {
		case 'r':
			for (i = R_EAX; i <= R_EDI; i++)
				printf("%s\t0x%08x\t%10d\n", regsl[i], reg_l(i), reg_l(i));
			printf("%s\t0x%08x\t%10d\n", "eip", cpu.eip, cpu.eip);
			printf("[OF IF SF ZF CF] = [%d %d %d %d %d]\n", 
			       cpu.OF, cpu.IF, cpu.SF, cpu.ZF, cpu.CF);
			break;
		case 'w':
			print_all_wp();
			break;
		default:
			print_error("Argument Error: Argument should be r or w!");
	}
	return 0;
}

static int cmd_p(char *args) {
	char *expression = args; 
	if (!expression) {
		print_error("Argument Error: There should be an argument!");
		return 0;
	}
	
	bool success;
	uint32_t val = expr(expression, &success);
	if (success)
		printf("%s = %dU = %d = 0x%x\n", expression, val, (int)val, val);	
	return 0;	
}

static int cmd_x(char *args) {
	if (!args) {
		print_error("Argument Error: There should be at least two arguments!");
		return 0;
	}

	char *args_end = args + strlen(args),
			 *first_arg = strtok(NULL, " ");
	if (!first_arg){
		print_error("Argument Error: There should be at least two arguments!");
		return 0;
	}

	int N = atoi(first_arg);
	char *expression = first_arg + strlen(first_arg) + 1;
	if (expression >= args_end) {
		print_error("Argument Error: There should be at least two arguments!");
		return 0;
	}				

	bool success;
	vaddr_t addr = expr(expression, &success);
	if (!success)
		return 0;

	int i, j;
	for (i = 0; i < N; ++i) {
		printf("  0x%x:    ", addr);
		uint32_t value = vaddr_read(addr, 4);
		uint8_t *pbyte = (uint8_t *)&value;
		for (j = 0; j < 4; ++j)
			printf("%02x ", pbyte[j]);
		putchar('\n');
		addr += 4;
	}
	return 0;
}

static int cmd_w(char *args) {
	char *expr = args; 
	if (!expr) {
		print_error("Argument Error: There should be an expression!");
		return 0;
	}

	if (!insert_wp(expr))
		print_error("Error: Fail to insert watchpoint!");
	return 0;	
}

static int cmd_d(char *args) {
	char *arg = strtok(NULL, " ");
	if (!arg) {
		print_error("Argument Error: There should be an argument!");
		return 0;
	}

	int N = atoi(arg);
	if (!delete_wp(N)) 
		print_error("Error: Fail to delete watchpoint!");
	return 0;	
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
	{ "si", "Execute N instructions by step where N is 1 by default", cmd_si },
	{ "info", "Print some infomation about regs or watchpoiters", cmd_info },
	{ "p", "Print the value of an expreesion", cmd_p },
	{ "x", "Examine the memory", cmd_x },
	{ "w", "Set a watchpoint", cmd_w },
	{ "d", "Delete a watchpoint", cmd_d }
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
