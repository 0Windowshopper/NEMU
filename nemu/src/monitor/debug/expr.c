#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h> // 添加头文件以支持atoi函数


enum {
	NOTYPE = 256, 
EQ=257,
NEQ=258, 
AND=259,
OR=260,
NOT=261, 
HEX=262,
REG=263, 
NUM=264, 	
NEG, DEF

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces
	{"==", EQ},						// equal
	{"\\(",'('},					//left parenthesis
	{"\\)",')'},					//right parenthesis
	{"\\*",'*'},					// multiply
	{"/",'/'},						// division
	{"-",'-'},						//substraction
	{"\\+", '+'},					// plus
	{"!=",NEQ},						//not equal
	{"\\&\\&",AND},						//logical and
	{"\\|\\|",OR},						//logical or
	{"!",NOT},						//logical not
	{"0[xX][a-fA-F0-9]{0,8}",HEX},	//hex
	{"\\$[a-d][hl]|\\$[e]?[ax|dx|cx|bx|bp|si|di|sp]|\\$(eip)",REG},//register
	{"[0-9]{1,10}",NUM},			//number
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
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

			
	   Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */
                             switch(rules[i].token_type) {
					case 40:
						tokens[nr_token].type = 40;
						break;
					case 41:
						tokens[nr_token].type = 41;
						break;
					case 42:
						tokens[nr_token].type = 42;
						break;
					case 47:
						tokens[nr_token].type = 47;
						break;
					case 43:
						tokens[nr_token].type = 43;
						break;
					case 45:
						tokens[nr_token].type = 45;
						break;
					case 256:
						nr_token--;
						break;
					case 258:
						tokens[nr_token].type = 258;
						strcpy(tokens[nr_token].str,"!=");
						break;
					case 259:
						tokens[nr_token].type = 259;
						strcpy(tokens[nr_token].str,"&&");
						break;
					case 260:
						tokens[nr_token].type = 260;
						strcpy(tokens[nr_token].str,"||");
						break;
					case 261:
						tokens[nr_token].type = 261;
						break;
					case 262:
						tokens[nr_token].type = 262;
						strncpy(tokens[nr_token].str,&e[position-substr_len],substr_len);
						
						break;
					case 263:
						tokens[nr_token].type = 263;
						strncpy(tokens[nr_token].str,&e[position-substr_len],substr_len);
						break;
					case 264:
						tokens[nr_token].type = 264;
						strncpy(tokens[nr_token].str,&e[position-substr_len],substr_len);
						break;
					case 257:
						tokens[nr_token].type  = 257;
						strcpy(tokens[nr_token].str,"==");
						break;
					default:
						nr_token--; 
						break;
				}
				nr_token++;
				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}
	return true; 
}
bool check_parentheses(int p,int q)
{
	int left = 0;
	int flag = 0;
	if(tokens[p].type == 40)
	{
		left++;
		int i;
		for(i=p+1;i<=q;i++)
		{
			if(tokens[i].type==40)
			{
				left++;
			}
			else if(tokens[i].type==41)
			{
				left--;
				if(left==0&&i!=q)
				{
					flag++;
				}
				if(left<0)
				{
					assert(0);
				}
			}
		}
		if(flag==0&&left==0&&tokens[q].type==41)
		{
			return 1;
		}
		else if(left==0)
		{
			return 0;
		}
		else
		{
			assert(0);
		}
	}
	else return 0;
}


int find_dominant_operator(int p,int q)
{
	int j=p;
	int i=0;
	int pos[5]={-1,-1,-1,-1,-1};
	for(;j<=q;j++)
	{
		if(tokens[j].type<262&&tokens[j].type!=261)
		{
			if(tokens[j].type==40)
			{
				i++;
				for(j=j+1;tokens[j].type!=41||i!=1;j++)
				{
					if(tokens[j].type==40)
					{
						i++;
					}
					if(tokens[j].type==41)
					{
						i--;
					}
				}
				i=0;
			}
			else if(tokens[j].type==NOT||tokens[j].type==DEF||tokens[j].type==NEG)pos[4]=j;
			else if(tokens[j].type=='*'||tokens[j].type=='/')pos[3]=j;
			else if(tokens[j].type=='+'||tokens[j].type=='-')pos[2]=j;
			else if(tokens[j].type==EQ||tokens[j].type==NEQ)pos[1]=j;
			else if(tokens[j].type==AND||tokens[j].type==OR)pos[0]=j;
		}
	}
	int k;
	for(k=0;k<5;k++)
	{
		if(pos[k]!=-1)return pos[k];
	}
	assert(0);
}

int eval(int p,int q)
{
	int i=0;
	if(p>q)
	{
		assert(0);
	}
	else if(p==q)
	{
		if(tokens[p].type==264)
		{
			i=atoi(tokens[q].str);
			return i;
		}
		else if(tokens[p].type == 262)
		{
			i=atoi(tokens[q].str);
			return i;
		}
		else if(tokens[p].type == 263)
		{
			int j=0,l=1,w=1,b=0;
			for(;j<8&&l!=0&&w!=0&&b!=0;j++)
			{
				l=strcmp(tokens[p].str,regsl[j]);
				w=strcmp(tokens[p].str,regsw[j]);
				b=strcmp(tokens[p].str,regsb[j]);
			}
			if(l==0)
			{
				i=reg_l(j);
				return i;
			}
			else if(w==0)
			{
				i=reg_w(j);
				return i;
			}
			else if(b==0)
			{
				i=reg_b(j);
				return i;
			}
			else{
				return cpu.eip;
			}
			if(j==8)
			{
				assert(0);
			}
		}
		else {
			assert(0);
		}
	}
	else if(check_parentheses(p,q))
	{
		return eval(p+1,q-1);
	}
	else
	{
		int op,val1,val2;
		op=find_dominant_operator(p,q);
		switch(tokens[op].type)
		{
			case NEG:
				i=0-eval(p+1,q);
				return i;
			case DEF:
				i=swaddr_read(eval(p+1,q),4);
				return i;
			case NOT:
				i=eval(p+1,q);
				return (i==0)?(1):(0);
		}
		val1=eval(p,op-1);
		val2=eval(op+1,q);
		switch(tokens[op].type)
		{
			case '+':
				return val1+val2;
			case '-':
				return val1-val2;
			case '*':
				return val1*val2;
			case '/':
				return val1/val2;
			case 257:
				return (val1==val2)?(1):(0);
			case 258:
				return (val1==val2)?(0):(1);
			case 259:
				return (val1&&val2)?(1):(0);
			case 260:
				return (val1||val2)?(1):(0);
			default:
				assert(0);
		}
	}
	return 0;
}
		
uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	int i=0;
	for(i=0;i<nr_token;i++){
		if(tokens[i].type=='*'){
			if(i==0)
				tokens[i].type=DEF;
			else if(tokens[i-1].type==264||tokens[i-1].type==262||tokens[i-1].type==41)continue;
			else tokens[i].type=DEF;
		}
		if(tokens[i].type=='-')
		{
			if(i==0)
				tokens[i].type=NEG;
			else if(tokens[i-1].type==264||tokens[i-1].type==262||tokens[i-1].type==41)continue;
			else tokens[i].type=NEG;
		}
	}
	*success=true;
	return eval(0,nr_token-1);
}

