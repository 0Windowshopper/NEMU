#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {//类似宏定义的方法定义运算操作
	NOTYPE = 256, 
	DECNUM, HEXNUM,
	REG,
	NEG, POINT,
	EQ, NEQ, LEQ, GEQ,
	OR, AND,

	/* TODO: Add more token types */

};

static struct rule {//根据正则表达式
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},							// spaces
	{"\\+", '+'},								// plus
	{"\\-", '-'},
	{"\\*", '*'},
	{"\\/", '/'},
	{"\\%", '%'},
 	{"\\(", '('},								//左括号
	{"\\)", ')'},								//左括号	


	{"\\$[a-z]{2,3}", REG},      			 	//寄存器
	{"\\0[xX][0-9a-fA-F]{1,10}", HEXNUM},    	//十六进制至少一位，最多十位
	{"[0-9]{1,10}", DECNUM},                	//数字,最多十位

	{"==", EQ},									// equal
	{"!=", NEQ},                  				//not equal
	{"<=", LEQ}, 								//less equal
	{">=", GEQ},								//greater equal
	{">", '>'},									//greater
	{"<", '<'},									//less
	{"\\=", '='},								//赋值

	{"\\!", '!'},								//非
	{"\\|\\|", OR},                 			//或
	{"\\&\\&", AND},							//与
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * ThePOINTore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
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
	
	nr_token = 0;//token到的元素数目

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				// Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);
				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */
				int j;
				for(j = 0; j < 32; ++j){//清空
					tokens[nr_token].str[j] = '\0';
				}
				
				int ret;
				switch(rules[i].token_type) {//根据enum和rule进行操作
					case 256://empty
						break;
					case DECNUM:
					case HEXNUM:
						tokens[nr_token].type = rules[i].token_type;
						ret = sprintf(tokens[nr_token].str, "%.*s", substr_len, substr_start);
						Assert(ret == substr_len, "Number too large error");
						nr_token++;
						break;
					case REG:
						tokens[nr_token].type = rules[i].token_type;
						sprintf(tokens[nr_token].str, "%.*s", substr_len - 1, substr_start + 1);
						nr_token++;
						break;			
					case EQ:
					case NEQ:
					case LEQ:
					case GEQ:
					case POINT:
					case AND:
					case OR:
					case NEG:
					case '+':
					case '-':
					case '*':
					case '/':
					case '!':
					case '(':
					case ')':
					case '%':
					case '>':
					case '<':
					case '=':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;					
					default: panic("please implement me");//强行断言退出
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
		if(nr_token > 12800){
			printf("The expression is too long:\n %s.\n",e);
			return false;
		}
	}
	return true; 
}

bool check_bracket(int p, int q){//判断括号是不是合法
	int i;
	int sum_bracket = 0;

	for(i = p; i <= q; ++i){
		if(tokens[i].type == '('){
			sum_bracket++;
		}
		if(tokens[i].type == ')'){
			sum_bracket--;
		}
		if(sum_bracket < 0){
			return false;
		}
	}
	return (sum_bracket == 0);
}
bool check_parentheses(int p, int q){//判断是否包含在一个括号中 //要保证传进来的p<q //传进来的一定是合法的
	if(tokens[p].type == '(' && tokens[q].type == ')'){
		int i;
		int sum_bracket = 0;
		for(i = p; i <= q; ++i){
			if(tokens[i].type == '('){
				sum_bracket++;
			}
			if(tokens[i].type == ')'){
				sum_bracket--;
			}
			if(i != q && sum_bracket == 0){
				return false;
			}
		}
		return true;
	}
	return false;
}

static int get_priority(int opt){//优先级越高，返回值越大
	// Log("tyep = %d\n", opt);
	switch(opt){
		case '+':
		case '-':
			return 4;
		case '*':
		case '/':
		case '%':
			return 3;
		case '=':
			return 14;
		case '<':
		case '>':
		case LEQ:
		case GEQ:
			return 6;
		case EQ:
		case NEQ:
			return 7;
		case AND:
			return 11;
		case OR:
			return 12;
		case '!':
		case NEG:
		case POINT:
			return 2;
		case '(':
			return 1;
		case ')':
			return 0;
		default:
			return -1;
	}
	return -1;
}

static uint32_t get_dominat_pos(int p, int q){//要保证传进来的p<q 并且合法
	int i;
	uint32_t pos = 0;
	int pri = -1;
	int sum_bracket = 0;
	for(i = p; i <= q; ++i){
		if(tokens[i].type == '('){
			sum_bracket ++;
		}
		if(tokens[i].type == ')'){
			sum_bracket --;
		}
		if(sum_bracket == 0){//只有不在一个括号中的先分
			int now_pri = get_priority(tokens[i].type);
			// Log("i = %d", i);
			// Log("pri = %d", pri);
			// Log("pos = %d", pos);
			// Log("now_pri = %d", now_pri);
			if(now_pri >= pri){
				pri = now_pri;
				pos = i;
			}
			// Log("After, i = %d", i);
			// Log("After, pri = %d", pri);
			// Log("After, pos = %d", pos);
			// Log("After, now_pri = %d", now_pri);
		}
	}
	return pos;
}

static uint32_t calc(uint32_t x, uint32_t y, int opt){//计算单个的运算
	switch(opt){
		case '+':
			return x + y;
		case '-':
			return x - y;
		case '*':
			return x * y;
		case '/':
			return x / y;
		case '%':
			return x % y;
		case '=':
			return x = y;
		case '<':
			return x < y;
		case '>':
			return x > y;
		case AND:
			return x && y;
		case OR:
			return x || y;
		case EQ:
			return x == y;
		case NEQ:
			return x != y;
		case LEQ:
			return x <= y;
		case GEQ:
			return x >= y;
		default:
			return -1;		
	}
	return -1;
}

static uint32_t eval(int p, int q, bool *success){
	// Log("p = %d, q = %d\n", p, q);
	if(p > q){
		// Log("p = %d, q = %d\n", p, q);
		panic("Bad Expression");
		*success = false;
	}else{
		if(p == q){//锁定单个的字符
			uint32_t val;
			switch(tokens[p].type){
				case DECNUM://十进制数
					sscanf(tokens[p].str, "%u", &val);
					// Log("DECNUM");
					return val;
				case HEXNUM://十六进制数
					sscanf(tokens[p].str, "%x", &val);
					return val;
				case REG://寄存器
					if(strcmp(tokens[p].str, "eip") == 0){
						return cpu.eip;
					}
					int i;
					// Log("REG = %s",tokens[p].str);
					
					for(i = 0; i < 8; ++i){//从长到短依次匹配寄存器
						if(strcmp(regsl[i], tokens[p].str) == 0){//!
							// Log("success reg_l");
							return reg_l(i);
						}
						if(strcmp(regsw[i], tokens[p].str) == 0){
							
							return reg_w(i);
						}
						if(strcmp(regsb[i], tokens[p].str) == 0){
							return reg_b(i);
						}
					}
					break;
				default:
					break;
			}
			panic("Bad Expression");
			*success = false;		
		}else if(check_parentheses(p, q)){//是完整的包含在了两个合法的括号之间
			return eval(p + 1, q - 1, success);
		}else {
			uint32_t pos = get_dominat_pos(p, q);
			// Log("pos = %d\n", pos);
			if(get_priority(tokens[pos].type) == 2){//! - *
				uint32_t val = eval(pos + 1, q, success);
				if(*success){
					switch(tokens[p].type){
						case NEG:
							return -val;
						case POINT:
							return swaddr_read(val, 4);
						case '!':
							return !val;
						default:
							break;
					}		
				}
				panic("Bad Expression");
				success = false;
			}else{
				uint32_t lval = eval(p, pos - 1, success);
				if(success == false){ 
					return -1;
				}
				// Log("Success finish lval = %d", lval);
				uint32_t rval = eval(pos + 1, q, success);
				if(success == false){
					return -1;
				}
				// Log("Success finish lval = %d", rval);
				Assert(!(rval == 0 && (tokens[pos].type == '/' || tokens[pos].type == '%')), "Error: divide 0!");
				uint32_t val = calc(lval, rval, tokens[pos].type);
				return val;
			}
		}
	}
	return -1;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}
	// Log("success make_token");
	if(check_bracket(0, nr_token - 1)){
		// Log("Legal");
		int i;
		for(i = 0;i < nr_token;i ++){//处理指针和负号
			if(tokens[i].type == '-' && (!i||get_priority(tokens[i-1].type)>0))
				tokens[i].type = NEG;
			else if(tokens[i].type == '*' && (!i||get_priority(tokens[i-1].type)>0))
				tokens[i].type = POINT;
		}
		// Log("nr_token = %d\n", nr_token);
		uint32_t val = eval(0, nr_token - 1, success);
		// Log("FINISH EVAL, success = %d\n", *success);
		if(*success == true) {
			// Log("Duccess calculated!");
			return val;
		}
	}
	/* TODO: Insert codes to evaluate the expression. */
	*success = false;
	panic("please implement me");
	return 0;
}

