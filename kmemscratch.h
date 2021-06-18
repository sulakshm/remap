#ifndef _KMEM_SCRATCH_H_
#define _KMEM_SCRATCH_H_

int kmemscratch_init(void);
void kmemscratch_exit(void);

int getpfn(unsigned i, unsigned long *pfn);
struct page* getpage(unsigned i);
void* getaddr(unsigned i);


#endif /* _KMEM_SCRATCH_H_ */
