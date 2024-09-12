#include "cpu/exec/template-start.h"

#define instr mov

static void do_execute() {
	OPERAND_W(op_dest, op_src->val);
	print_asm_template2();
}

make_instr_helper(i2r)
make_instr_helper(i2rm)
make_instr_helper(r2rm)
make_instr_helper(rm2r)

make_helper(concat(mov_a2moffs_, SUFFIX)) {
	swaddr_t addr = instr_fetch(eip + 1, 4);
	MEM_W(addr, REG(R_EAX), R_DS);

	print_asm("mov" str(SUFFIX) " %%%s,0x%x", REG_NAME(R_EAX), addr);
	return 5;
}

make_helper(concat(mov_moffs2a_, SUFFIX)) {
	swaddr_t addr = instr_fetch(eip + 1, 4);
	REG(R_EAX) = MEM_R(addr, R_DS);

	print_asm("mov" str(SUFFIX) " 0x%x,%%%s", addr, REG_NAME(R_EAX));
	return 5;
}

#if DATA_BYTE == 2
make_helper(mov_cr2r){
	uint8_t modrm= instr_fetch(eip + 1,1);
	uint8_t cr = (modrm >> 3) & 7; // reg
	uint8_t reg = modrm & 7; // r/m
	switch (cr){
		case 0:
			reg_l(reg) = cpu.cr0.val;
			print_asm("mov %%cr0,%%%s", regsl[reg]);
			break;
		case 3:
			reg_l(reg) = cpu.cr3.val;
			print_asm("mov %%cr3,%%%s", regsl[reg]);
			break;
		default:
			break;
	}
	return 2;
}
void init_tlb();
make_helper(mov_r2cr){
	uint8_t modrm= instr_fetch(eip + 1, 1);
	uint8_t cr = (modrm >> 3) & 7; // reg
	uint8_t reg = modrm & 7; // r/m
	switch (cr){
		case 0:
			cpu.cr0.val = reg_l(reg);
			print_asm("mov %%%s,%%cr0", regsl[reg]);
			break;
		case 3:
			init_tlb();
			cpu.cr3.val = reg_l(reg);
			print_asm("mov %%%s,%%cr3", regsl[reg]);
			break;
		default:
			break;
	}
	return 2;
}

make_helper(concat(mov_sr2rm_, SUFFIX)) {
	int len = decode_r2rm_w(eip + 1);
	write_operand_w(op_dest, cpu.sr[op_src->reg].val);
	print_asm("mov  %%%s,%s", sregs[op_src->reg], op_dest->str);
	return len + 1;
}
union {
	SegDesc seg;
	uint32_t val[2];
}Seg;
make_helper(concat(mov_rm2sr_, SUFFIX)) {
	int len = decode_rm2r_w(eip + 1);
	cpu.sr[op_dest->reg].val = op_src->val;
	lnaddr_t addr = cpu.gdtr.base + 8 * cpu.sr[op_dest->reg].index;
	int i;
	for(i = 0;i < 2;i++)
		Seg.val[i] = lnaddr_read(addr + 4 * i, 4);
	cpu.sr[op_dest->reg].cache.base = Seg.seg.base_15_0 + (Seg.seg.base_23_16 << 16) + (Seg.seg.base_31_24 << 24);
	cpu.sr[op_dest->reg].cache.limit = Seg.seg.limit_15_0 + (Seg.seg.limit_19_16 << 16);
	print_asm("mov  %s,%%%s", op_src->str, sregs[op_dest->reg]);
	return len + 1;
}
#endif

#include "cpu/exec/template-end.h"
