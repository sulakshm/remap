#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gfp.h>
#include <linux/highmem.h>

#include "kmemscratch.h"


struct page* scratchpad[16] = {NULL};
void *maddr[16] = {0};


int getpfn(unsigned i, unsigned long *pfn)
{
	if (i >= 16 || scratchpad[i] == NULL) return -EINVAL;
	*pfn = page_to_pfn(scratchpad[i]);
	return 0;
}

struct page* getpage(unsigned i)
{
	if (i >= 16) return NULL;

	return scratchpad[i];
}

void* getaddr(unsigned i)
{
	if (i >= 16) return 0;

	return maddr[i];
}

int scratch_write(unsigned i, unsigned long magic)
{
	struct page *p = scratchpad[i];
	unsigned limit, x;
	unsigned long *base;

	if (i >= 16 || p == NULL)
		return -EINVAL;

	limit = PAGE_SIZE / sizeof(unsigned long);

	base = kmap_atomic(p);
	for (x=0; x < limit; x++) {
		base[x] = magic;
	}

	kunmap_atomic(base);
	//maddr[i] = base;

	return 0;
}


const unsigned long pattern[16] = {
	0x1111111111111111UL,
	0x2222222222222222UL,
	0x3333333333333333UL,
	0x4444444444444444UL,
	0x5555555555555555UL,
	0x6666666666666666UL,
	0x7777777777777777UL,
	0x8888888888888888UL,
	0x9999999999999999UL,
	0xaaaaaaaaaaaaaaaaUL,
	0xbbbbbbbbbbbbbbbbUL,
	0xccccccccccccccccUL,
	0xddddddddddddddddUL,
	0xeeeeeeeeeeeeeeeeUL,
	0xffffffffffffffffUL,
	0x0000000000000000UL};

int kmemscratch_init(void)
{
	int i;

	for (i=0; i<16; i++) {
		struct page *p = alloc_page(GFP_KERNEL);
		BUG_ON(!p);

		scratchpad[i] = p; // get_page(p);

		scratch_write(i, pattern[i]);
	}

	return 0;
}


void kmemscratch_exit(void)
{
	int i;

	for (i=0; i<16; i++) {
		struct page *p = scratchpad[i];

		if (p != NULL) {
			//kunmap(maddr[i]);
			// put_page(p);
			__free_pages(p, 0);
		}
	}
}
