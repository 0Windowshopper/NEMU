#ifndef __STD_H_
#define __STD_H_

#include "cpu/exec/helper.h"

make_helper(std){
	cpu.eflags.DF = 1;
	print_asm("std");
	return 1;
}

#endif
