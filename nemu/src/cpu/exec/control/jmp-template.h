#include "cpu/exec/template-start.h"

#define instr jmp

static void do_execute() {
	cpu.eip += op_src->val;
	print_asm(str(instr) " %x", cpu.eip + 1 + DATA_BYTE);
}

make_instr_helper(si)
#if DATA_BYTE == 4

make_helper(jmp_rm_l) {
	int len = decode_rm_l(eip + 1);
	cpu.eip = op_src->val - (len + 1);
	print_asm(str(instr) " *%s", op_src->str);
	return len + 1;
}

union {
	SegDesc seg;
	uint32_t val[2];
}Seg;
make_helper(ljmp) {
	uint32_t val1 = instr_fetch(eip + 1, 4);
	uint16_t val2 = instr_fetch(eip + 5, 2);
	cpu.eip = val1 - 7;
	cpu.CS.val = val2;
	lnaddr_t addr = cpu.gdtr.base + 8 * cpu.CS.index;
	int i;
	for(i = 0;i < 2;i++)
		Seg.val[i] = lnaddr_read(addr + 4 * i, 4);
	cpu.CS.cache.base = Seg.seg.base_15_0 + (Seg.seg.base_23_16 << 16) + (Seg.seg.base_31_24 << 24);
	cpu.CS.cache.limit = Seg.seg.limit_15_0 + (Seg.seg.limit_19_16 << 16);
	
	print_asm("ljmp $0x%x,$0x%x", val2, val1);
	return 7;
}
#endif

#include "cpu/exec/template-end.h"
