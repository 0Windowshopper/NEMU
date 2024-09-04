#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint32_t);

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

uint32_t nr_exp = 0;
int nr_wp = 0;

static int cmd_c(char *args) {
	cpu_exec(-1);
	return 0;
}

static int cmd_q(char *args) {
	return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
static struct {
	char *name;
	char *description;
	int (*handler) (char *);
} cmd_table [] = {
	{ "help", "Display informations about all supported commands", cmd_help },
	{ "c", "Continue the execution of the program", cmd_c },
	{ "q", "Exit NEMU", cmd_q },
	{ "si", "One step", cmd_si },
	{ "info", "info  r : Print register status, \ninfo w :Print watchpoints.",cmd_info},
	{ "p", "Print the expression", cmd_p},
	{ "x", "Scan the RAM", cmd_x},
	{ "w", "Set a watchpoint for an expression.", cmd_w},
	{ "d", "Delete a watchpoint.", cmd_d},
	/* TODO: Add more commands */
};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
	/* extract the first argument */
	char *arg = strtok(NULL, " ");
	int i;

	if(arg == NULL) {
		/* no argument given */
		for(i = 0; i < NR_CMD; i ++) {
			printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
		}
	}
	else {
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(arg, cmd_table[i].name) == 0) {
				printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
				return 0;
			}
		}
		printf("Unknown command '%s'\n", arg);
	}
	return 0;
}

static int cmd_si(char *args){//单步执行
	char *si_second = strtok(NULL, " ");
	int step = 0;
	int i;
	if(si_second == NULL){//缺省是1
		cpu_exec(1);
		printf("0x%x\n",cpu.eip);
		return 0;
	}
	sscanf(si_second, "%d", &step);//step存储执行的指令数目
	if(step <= 0){
		printf("Error: Less equal than 0!\n");
		return 0;
	}
	if(step > (uint32_t)(-1)){//-1在无符号下的意义是32位最大的数
		printf("Error: Numeric constant too large.\n");
		return 0;
	}
	for(i = 0; i < step; ++i){
		cpu_exec(1);
	}
	printf("0x%x\n",cpu.eip);
	return 0;
}

static int cmd_info(char *args){//打印寄存器的内容
	char *info_second = strtok(NULL, " ");
	int i;
	if(strcmp(info_second, "r") == 0){
		for(i = 0; i < 4; ++i){
			printf("$%s (0x%08x) ", regsl[i], cpu.gpr[i]._32);
			// printf("%s\t\t", regsl[i]);//使用\t对齐输出
			// printf("0x%08x\t\t%d\n", cpu.gpr[i]._32, cpu.gpr[i]._32);
		}
		// printf("eip\t\t0x%08x\t\t%d\n", cpu.eip, cpu.eip);
		printf("\n");
		return 0;
	}else if(strcmp(info_second, "w") == 0){
		print_wp();	
	}else{
		printf("Undefined info command: \"%s\".  Try \"help info\".\n",args);
		return 0;
	}
	return 0;
}

static int cmd_x(char *args){//扫描内存
	char *x_second = strtok(NULL, " ");
	char *x_third = strtok(NULL, " ");
	
	int step = 0;
	swaddr_t address;
	sscanf(x_second, "%d", &step);
	sscanf(x_third, "%x", &address);
	
	int i, j = 0;
	for(i = 0; i < step; ++i){
		// if(j % 4 == 0){
			// printf("0x%x:", address);//
		// }
		printf("0x%08x ", swaddr_read(address, 4));
		address += 4;
		++j;
		if(j % 4 == 0){
			puts("");
		}
	}
	puts("");
	return 0;
}

static int cmd_p(char *args){
	bool success = true;
	uint32_t val = expr(args , &success);
	if(success){
		printf("%u\n", val);
		// printf("$%d = %u (0x%x)\n", nr_exp, val, val);
		nr_exp++;
	}
	return 0;
}

static int cmd_w(char *args){
	bool success = true;//用于表示表达式求值是否成功
	uint32_t val = expr(args , &success);
	if(!success){//检查 success 变量是否为 false，如果是，则表示表达式求值失败
		printf("Error: Invalid expression!\n");
		return 0;
	}
	WP *wp = new_wp();
	if(wp != NULL){
		strcpy(wp->exp,args);
		wp->old_val = val;
		wp->new_val = val;
		wp->NO = nr_wp;
		// printf("watchpoint %d : %s\n", nr_wp, args);
		nr_wp++;//增加 nr_wp 的值，为下一个监视点编号做准备
	}else{
		printf("Error: Watchpoint Exceed!\n");
	}
	return 0;
}	

static int cmd_d(char *args){
	int no;
	if(sscanf(args,"%d",&no) != 1){
		printf("Invalid Argumnts!");
		return 0;
	}
	delete_wp(no);	
	return 0;
}

void ui_mainloop() {
	while(1) {
		char *str = rl_gets();
		char *str_end = str + strlen(str);

		/* extract the first token as the command */
		char *cmd = strtok(str, " ");
		if(cmd == NULL) { continue; }

		/* treat the remaining string as the arguments,
		 * which may need further parsing
		 */
		char *args = cmd + strlen(cmd) + 1;
		if(args >= str_end) {
			args = NULL;
		}

#ifdef HAS_DEVICE
		extern void sdl_clear_event_queue(void);
		sdl_clear_event_queue();
#endif

		int i;
		for(i = 0; i < NR_CMD; i ++) {
			if(strcmp(cmd, cmd_table[i].name) == 0) {
				if(cmd_table[i].handler(args) < 0) { return; }
				break;
			}
		}

		if(i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
	}
}
