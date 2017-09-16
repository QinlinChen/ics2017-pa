#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256,
 	TK_EQ, TK_ADD, TK_SUB, TK_MUL, TK_DIV,
	TK_DINT, TK_LPAREN, TK_RPAREN

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

	{" +", TK_NOTYPE},    // spaces
	{"\\(", TK_LPAREN},		// left parentheses
	{"\\)", TK_RPAREN},		// right parentheses
	{"[0-9]+", TK_DINT},	// decimal integer
	{"\\+", TK_ADD},			// add
	{"\\-", TK_SUB},			// minus
	{"\\*", TK_MUL},			// multiply
	{"/", TK_DIV},				// divide
	{"==", TK_EQ}         // equal
	
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
					if (nr_token >= 32) {
						print_error("Error: nr_token is more than the limitation of 31");
						return false;
					}	
					if (substr_len >= 32) {
						print_error("Error: the length of substring is more than 31!");
						return false;
					}
					/* cases that ignored*/
					case TK_NOTYPE:
						break;
					/* cases that needn't save the substring*/	
					case TK_ADD: case TK_SUB: case TK_MUL: case TK_DIV:
					case TK_LPAREN: case TK_RPAREN:
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;	
						break;
					/* cases that need save the substring*/
					case TK_DINT:
						tokens[nr_token].type = rules[i].token_type;
						memcpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						nr_token++;	
						break;

          default: 
						break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool parentheses_are_matched(int p, int q) {
	int stack_top = 0, i;
	for(i = p; i <= q; ++i) 
		if (tokens[i].type == TK_LPAREN) 
			stack_top++;
		else if (tokens[i].type == TK_RPAREN) {
			if (stack_top == 0)
				return false;
			stack_top--;
		}
	return true;
}

bool check_parentheses(int p, int q) {
	return (tokens[p].type == TK_LPAREN) && (tokens[q].type == TK_RPAREN);
}

int eval(int p, int q, bool *success) {
	if (!(*success))
		return 0;
	if (p > q) {
		print_error("Error: Bad expression where p > q");
		*success = false;
		return 0;	
	}	
	else if (p == q) {
		return atoi(tokens[p].str);
	}	
	else if (check_parentheses(p, q) == true){
		return eval(p + 1, q - 1, success);
	}
	else {
	
	}
	return 1;
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
		print_error("Error: Fail to make tokens!");
    return 0;
  }

	if (!parentheses_are_matched(0, nr_token - 1)) {
		*success = false;
		print_error("Syntex Error: Parentheses are not matched!");
		return 0;
	} 

	int i;
	for (i = 0; i < nr_token; ++i) 
		printf("tokens[%d]: (%d, %s)\n", i, tokens[i].type, tokens[i].str);

	*success = true;
  return 0;
}
