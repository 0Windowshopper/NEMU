//#include "common.h"
#include "nemu.h"
uint32_t cache_read(hwaddr_t, size_t);
void cache_write(hwaddr_t, size_t, uint32_t);
hwaddr_t page_translate(lnaddr_t );
/* Memory accessing interfaces */


uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	return cache_read(addr, len) & (~0u >> ((4 - len) << 3));
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	cache_write(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	assert(len == 1 || len == 2 || len == 4);
	uint32_t _size = (1 << 12), _mask = _size - 1;
	if ((addr & _mask) + len > _size) {
		/* this is a special case, you can handle it later. */
		// assert(0);
		int i;
		uint32_t ret = 0;
		for(i = 0;i < len; i++)
		      ret = (ret << 8) + lnaddr_read(addr + i, 1);
		return ret;
	}
	else {
		hwaddr_t hwaddr = page_translate(addr);
		return hwaddr_read(hwaddr, len);
	}
}
void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	assert(len == 1 || len == 2 || len == 4);
	uint32_t _size = (1 << 12), _mask = _size - 1;
	if ((addr & _mask) + len > _size) {
	/* this is a special case, you can handle it later. */
//		assert(0);
		int i;
		for(i = 0;i < len; i++)
		      lnaddr_write(addr + i, 1, data & 0xff),
			data >>= 8;
		
	}
	else {
		hwaddr_t hwaddr = page_translate(addr);
		return hwaddr_write(hwaddr, len, data);
	}
}

lnaddr_t seg_translate(swaddr_t addr, uint8_t sreg){
	if(!cpu.cr0.protect_enable)return addr;
	return cpu.sr[sreg].cache.base + addr;
}

uint32_t swaddr_read(swaddr_t addr, size_t len, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_t lnaddr = seg_translate(addr, sreg);
	return lnaddr_read(lnaddr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint32_t data, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_t lnaddr = seg_translate(addr, sreg);
	lnaddr_write(lnaddr, len, data);
}

