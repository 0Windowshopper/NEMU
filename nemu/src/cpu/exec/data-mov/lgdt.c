#include "cpu/exec/helper.h"
#define instr lgdt

make_helper(lgdt){
	int len = decode_rm_w(eip + 1);
	cpu.gdtr.limit = lnaddr_read(op_src->addr, 2);
	cpu.gdtr.base = lnaddr_read(op_src->addr + 2, 4);
	print_asm(str(instr) " 0x%x", op_src->addr);
	return len + 1;
}

