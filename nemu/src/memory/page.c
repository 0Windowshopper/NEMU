#include "nemu.h"
#include "../../../lib-common/x86-inc/mmu.h"
#include <stdlib.h>
#include <time.h>
typedef union{
	struct{
		uint32_t offset	:12;
		uint32_t page	:10;
		uint32_t dir	:10;
	};
	uint32_t val;
}page_addr;

void init_tlb(){
	int i;
	for(i = 0;i < 64; i++)
	      tlb[i].valid = 0;
}

hwaddr_t page_translate(lnaddr_t lnaddr){
	if(!(cpu.cr0.protect_enable && cpu.cr0.paging))
	      return lnaddr;
	uint32_t tag = lnaddr >> 12;
	page_addr addr;
	addr.val = lnaddr;
	int i;
	for(i = 0;i < 64; i++)
	      if(tlb[i].valid && tlb[i].tag == tag)
		    return (tlb[i].addr << 12) + addr.offset;
	PDE pde;
	pde.val = hwaddr_read(addr.dir * 4 + (cpu.cr3.page_directory_base << 12), 4);
	assert(pde.present);
	PTE pte;
	pte.val = hwaddr_read(addr.page * 4 + (pde.page_frame << 12), 4);
	assert(pte.present);
	for(i = 0;i < 64; i++)
	      if(!tlb[i].valid)
		    break;
	srand(time(0));
	if(i == 64) i = rand() % 64;
	tlb[i].valid = 1;
	tlb[i].tag = tag;
	tlb[i].addr = pte.page_frame;

	return (pte.page_frame << 12) + addr.offset;
}
