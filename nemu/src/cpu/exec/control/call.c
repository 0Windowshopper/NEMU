#include "cpu/exec/helper.h" 

#define DATA_BYTE 2
#include "call-template.h"
#undef DATA_BYTE

#define DATA_BYTE 4
#include "call-template.h"
#undef DATA_BYTE

make_helper_v(call_si)
make_helper_v(call_rm)
