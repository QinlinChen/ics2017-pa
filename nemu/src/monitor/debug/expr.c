#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
  TK_NOTYPE = 256,
	/* interger and registers */
	TK_DINT, TK_HINT, TK_REG,
	/* parentheses */
	TK_LPAREN, TK_RPAREN,

	TK_OP_BEG,				// operator definition BEGIN
	
	TK_BOP_BEG,				// binary operator definition BEGIN	
	TK_OR,						// priority 1
	TK_AND,						// priority 2
	TK_BOR,						// priority 3
	TK_BXOR,					// priority 4
	TK_BAND,					// priority 5
	TK_EQ, TK_NEQ,		// priority 6
	TK_L, TK_LE, TK_G, TK_GE,		// priority 7
	TK_ADD, TK_SUB,							// priority 8
	TK_MUL, TK_DIV, TK_MOD,			// priority 9
	TK_BOP_END,				// binary operator definition END
	
	TK_UOP_BEG,				// unary operator definition BEGIN
	TK_BNOT, TK_NOT, TK_DEREF, TK_NEG,		//priority 10
	TK_UOP_END,				// unary operator definition END

	TK_OP_END					// operator defintion end

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* Pay attention to the precedence level of different rules. */

	{" +", TK_NOTYPE},			// spaces
	{"\\(", TK_LPAREN},			// left parentheses
	{"\\)", TK_RPAREN},			// right parentheses
	
	{"0x[0-9a-fA-F]+", TK_HINT},	// hexadecimal integer which is prior to DINT
	{"[0-9]+", TK_DINT},		// decimal integer
	
	{"\\$(e?[a-d][x]|e?[sb][p]|e?[sd][i]|[a-d][hl]|eip)", TK_REG},		//regs

	{"&{2}", TK_AND},				// logic and
	{"\\|{2}", TK_OR},			// logic or

	{"&", TK_BAND},					// bit and
	{"\\|", TK_BOR},				// bit or
	{"\\^", TK_BXOR},				// bit xor

	{"==", TK_EQ},					// equal
	{"!=", TK_NEQ},					// not equal which is prior to DEREF

	{"<=", TK_LE},					// less and equal
	{"<", TK_L},						// less
	{">=", TK_GE},					// greater and equal
	{">", TK_G},						// greater

	{"\\+", TK_ADD},				// add
	{"\\-", TK_SUB},				// minus OR negate

	{"\\*", TK_MUL},				// multiply OR dereference
	{"/", TK_DIV},					// divide
	{"%", TK_MOD},					// mod

	{"!", TK_NOT},					// logic not
	{"~", TK_BNOT}					// bit not
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
				
				
        Log_write("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

        /* Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
				if (nr_token >= 32) {
					print_error("Error: nr_token is more than the limitation of 31");
					return false;
				}	
				if (substr_len >= 32) {
					print_error("Error: the length of substring is more than 31!");
					return false;
				}

				int type = rules[i].token_type;
				/* cases that need to save the substring */
				if (type >= TK_DINT && type <= TK_REG) {
					tokens[nr_token].type = rules[i].token_type;
					memcpy(tokens[nr_token].str, substr_start, substr_len);
					tokens[nr_token].str[substr_len] = '\0';
					nr_token++;	
				}
				/* cases that needn't save the substring */	
				else if (type >= TK_LPAREN && type <= TK_OP_END) {
					tokens[nr_token].type = rules[i].token_type;
					tokens[nr_token].str[0] = '\0';
					nr_token++;	
				}
				/* others ignored */
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

static bool parentheses_are_matched(int p, int q) {
	int stack_top = 0, i;
	for (i = p; i <= q; ++i) 
		if (tokens[i].type == TK_LPAREN) 
			stack_top++;
		else if (tokens[i].type == TK_RPAREN) {
			if (stack_top == 0)
				return false;
			stack_top--;
		}
	if (stack_top != 0)
		return false;
	return true;
}

static int find_corresponding_parenthesis(int p) {
	/* precondition:
	 * token[p].type == TK_LPAREN
	 * parentheses_are_matched(0, nr_token - 1) == true */
	int stack_top = 1;
	while (++p < nr_token) {
		if (tokens[p].type == TK_LPAREN)
			stack_top++;
		else if (tokens[p].type == TK_RPAREN) {
			stack_top--;
			if (stack_top == 0)
				break;	
		}
	}
	Assert(tokens[p].type == TK_RPAREN, "Fail to find corresponding parenthesis!");
	return p;
}

static bool check_parentheses(int p, int q) {
	return (tokens[p].type == TK_LPAREN) 
		&& (find_corresponding_parenthesis(p) == q);
}

static bool is_operator(int type) {
	return (type > TK_OP_BEG) && (type < TK_OP_END);
}

static bool is_binary_operator(int type) {
	return (type > TK_BOP_BEG) && (type < TK_BOP_END);
}

static bool is_unary_operator(int type) {
	return (type > TK_UOP_BEG) && (type < TK_UOP_END);
}

static int operator_priority(int type) {
	switch (type) {
		case TK_OR:   return 1;
		case TK_AND:  return 2;
		case TK_BOR:  return 3;
		case TK_BXOR: return 4;
		case TK_BAND: return 5;
		case TK_EQ:		case TK_NEQ:
			return 6;
		case TK_L:		case TK_LE:		case TK_G:			case TK_GE:
			return 7;
		case TK_ADD:	case TK_SUB:
			return 8;
		case TK_MUL:	case TK_DIV:	case TK_MOD:
			return 9;
		case TK_BNOT:	case TK_NOT:	case TK_DEREF:	case TK_NEG:
			return 10;
		default:
			return 0;
	}
}

static int dominant_operator(int p, int q) {
	int i, type, cur_dominant = -1, cur_priority = 1000;
	for (i = p; i <= q; ++i) {
		type = tokens[i].type;
		/* If find left parenthesis, then
		 * jump to the corresponding right parenthesis*/
		if (type == TK_LPAREN) 
			i = find_corresponding_parenthesis(i);
		/* If is operator, compare the priority*/
		else if (is_operator(type)) {
			int pri = operator_priority(type);
			if(pri <= cur_priority) {
				cur_dominant = i;
				cur_priority = pri;
			}
		}
	}	
	return cur_dominant;
}

static void parse_special_token() {
	int i;
	for (i = 0; i < nr_token; ++i) {
		if (tokens[i].type == TK_MUL 
				&& (i == 0 || is_operator(tokens[i - 1].type) 
					|| tokens[i - 1].type == TK_LPAREN))
			tokens[i].type = TK_DEREF;
		else if (tokens[i].type == TK_SUB
				&& (i == 0 || is_operator(tokens[i - 1].type)
				 	|| tokens[i - 1].type == TK_LPAREN))
			tokens[i].type = TK_NEG;
	}
}

static int find_reg_index(char *reg, const char *regs[8]) {
	int i;
	for (i = 0; i < 8; ++i) 
		if (strcmp(reg, regs[i]) == 0)
			return i;
	return -1;
}

static uint32_t regname_to_val(char *name) {
	int index;
	if (strcmp(name, "eip") == 0)
		return cpu.eip;
	else if (name[0] == 'e') {
		index = find_reg_index(name, regsl);
		Assert(index >= 0, "Error: %s doesn't exist!", name);
		return reg_l(index);
	}
	else {
		index = find_reg_index(name, regsw);
		if (index >= 0) 
			return reg_w(index);

		index = find_reg_index(name, regsb);
		Assert(index >= 0, "Error: %s doesn't exist!", name);
		return reg_b(index);
	}	
}

static int eval(int p, int q, bool *success) {
	if (!(*success))
		return -1;

	if (p > q) {
		*success = false;
		print_error("Syntex Error: Bad expression!");
		return -1;	
	}	

	/* Process DINT, HINT and REG */
	else if (p == q) {
		int type = tokens[p].type;
		char *str = tokens[p].str;

		if (type == TK_HINT) {
			int val;
			if (!str || (sscanf(str, "%x", &val) != 1)) {
				*success = false;
				print_error("Error: Fail to read hexadecimal number!");
				return -1;
			}
			return val;
		}
		else if (type == TK_DINT)
		  return atoi(str);
		else if (type == TK_REG) 
			return (int)regname_to_val(str + 1);	
		
		Assert(0, "Neither int nor hex int nor reg!");
	}	

	/* Throw away the parentheses */
	else if (check_parentheses(p, q) == true){
		return eval(p + 1, q - 1, success);
	}

	/* Process operators */
	else {
		int pos = dominant_operator(p, q);
		if (pos < 0) {
			*success  = false;
			print_error("Syntex Error: Fail to find dominant operator!");
			return -1;
		}

		int op = tokens[pos].type;
		/* Process binary operator */
		if (is_binary_operator(op)) {
			int lval = eval(p, pos - 1, success),
					rval = eval(pos + 1, q, success);
			if (!(*success)) 
				return -1;
		
			switch (op) {
				case TK_OR:  return lval || rval;
				case TK_AND: return lval && rval;
				case TK_BOR: return lval |  rval;
				case TK_BXOR:return lval ^  rval;
				case TK_BAND:return lval &  rval;
				case TK_EQ:  return lval == rval;
				case TK_NEQ: return lval != rval;
				case TK_LE:  return lval <= rval;
				case TK_GE:  return lval >= rval;
				case TK_L:   return lval <  rval;
				case TK_G:	 return lval >  rval;						 
				case TK_ADD: return lval +  rval;
				case TK_SUB: return lval -  rval;
				case TK_MUL: return lval *  rval;
				case TK_DIV:
					if (rval == 0) {
						*success = false;
						print_error("Divisor Error: Divisor is zero!");
						return -1;
					} 
					return lval / rval;
				case TK_MOD:
					if (rval == 0) {
						*success = false;
						print_error("Mod Error: mod is zero!");
						return -1;
					} 
					return lval % rval;
				default:
					assert(0);
			}
		}

		/* Process unary operator */
		else if (is_unary_operator(op)) {
			int rval = eval(pos + 1, q, success);
			if (!(*success)) 
				return -1;
			
			vaddr_t addr;
			uint32_t mem_val;

			switch (op) {
				case TK_BNOT: return ~rval;
				case TK_NOT:  return !rval;
				case TK_NEG:  return -rval;
				case TK_DEREF:
					addr = rval;
					mem_val = vaddr_read(addr, 4);
					return (int)mem_val;					
				default:
					assert(0);
			}
		}
		else
			Assert(0, "Neither binary operator nor unary operator!");
	}
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

	parse_special_token();

	*success = true;
	return eval(0, nr_token - 1, success);
}
