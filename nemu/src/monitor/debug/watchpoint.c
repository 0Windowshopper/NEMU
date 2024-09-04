#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

//创建一个新的 WP 结构体实例
WP* new_wp(){//申请一个监视点
	if(free_ == NULL) return NULL;
	WP *tmp_wp;
	tmp_wp = free_;
	free_ = free_ -> next;
	if(head == NULL){
		head = tmp_wp;
		tmp_wp->next = NULL;
	} else {//类似于链式前向星
		tmp_wp->next = head;
		head = tmp_wp;
	}
	return tmp_wp;
}

void free_wp(WP *wp){//释放
	if(wp == head){
		head = wp->next;
	}else{
		WP *p = head;
		while(p->next != wp){
			p = p->next;
		}
		p->next = wp->next;
	}
	if(free_ == NULL){
		free_ = wp;
		wp->next = NULL;
	}else{//类似链式前向星
		wp->next = free_;
		free_ = wp;
	}
}

void delete_wp(int no){
	WP *wp = head;
	while(wp != NULL){
		if(wp->NO == no){
			free_wp(wp);
			printf("Have deleted watchpoint %d.\n", no);
			return ;
		}
		wp = wp->next;
	}
	if(wp == NULL){
		printf("NO watchpoint %d !\n", no);
	}
	return ;
}

void print_wp(){
	printf("NO\tType      \tExpression\n");
	WP *wp = head;
	while(wp != NULL){
		printf("%d\twatchpoint\t%s\n",wp->NO,wp->exp);
		wp = wp->next;
	}
}

bool check_wp(swaddr_t eip){//检查wp是否发生了改变
	WP *wp = head;//定义了一个指向 WP 结构体的指针 wp，并将其初始化为指向监视点链表的头部
	bool success = false, wp_changed = false;
	while(wp != NULL){
		success = true;//将 success 变量设置为 true，表示假设表达式求值成功
		wp->new_val = expr(wp->exp, &success);//调用 expr 函数来计算当前监视点 wp 的表达式 wp->exp 的新值，并将结果存储在 wp->new_val 中
		assert(success);
		wp = wp->next;
	}
	wp = head;
	//遍历所有的监视点
	while(wp != NULL){//移动 wp 指针到下一个监视点
		if(wp->new_val != wp -> old_val){//发生了变化
			wp_changed = true;
			printf("Hint watchpoint %d at address 0x%.8x\n", wp->NO, eip);
			// printf("Hint watchpoint %d at address 0x%.8x , expr = %s\n\nOld value = %d (0x%x)\nNew value = %d (0x%x)\n", wp->NO, eip, wp->exp, wp->old_val, wp->old_val, wp->new_val, wp->new_val);
			wp->old_val = wp->new_val;
		}
		wp = wp->next;
	}
	return wp_changed;
}
/* TODO: Implement the functionality of watchpoint */

