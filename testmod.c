#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/miscdevice.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/uio.h>
#include <linux/mm.h>
#include <asm/mmu_context.h>

#include "kmemscratch.h"

struct testheader {
	int cmd;
};

static
long testmod_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case 0:
		printk("testmod_ioctl interruptible sleep\n");
		// sleep for 60 secs
		schedule_timeout_interruptible(msecs_to_jiffies(60000));
		break;
	case 1:
		printk("testmod_ioctl uninterruptible sleep\n");
		// sleep for 60 secs
		schedule_timeout_uninterruptible(msecs_to_jiffies(60000));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static
ssize_t testmod_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	//struct file *file = iocb->ki_filp;
	struct testheader th;
	size_t len = sizeof(th);

	printk("testmod_read_iter iter.count %ld", iter->count);
	if (copy_from_iter(&th, len, iter) != len) {
		printk("testmod_read_iter cannot copy header");
		return -EFAULT;
	}

	printk("testmod_read_iter header cmd %d\n", th.cmd);
	switch (th.cmd) {
	case 0:
		// sleep for 60 secs
		schedule_timeout_interruptible(msecs_to_jiffies(60000));
		break;
	case 1:
		// sleep for 60 secs
		schedule_timeout_uninterruptible(msecs_to_jiffies(60000));
		break;
	default:
		return -EINVAL;
	}

	return iter->count;
}

ssize_t testmod_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
	//struct file *file = iocb->ki_filp;
	struct testheader th;
	size_t len = sizeof(th);

	if (copy_from_iter(&th, len, iter) != len) {
		return -EFAULT;
	}

	switch (th.cmd) {
	case 0:
		// sleep for 60 secs
		schedule_timeout_interruptible(msecs_to_jiffies(60000));
		break;
	case 1:
		// sleep for 60 secs
		schedule_timeout_uninterruptible(msecs_to_jiffies(60000));
		break;
	default:
		return -EINVAL;
	}

	return iter->count;
}

static void testmod_vm_close(struct vm_area_struct *vma)
{
    struct file *file = vma->vm_file;
    // struct pxd_context *ctx = container_of(file->f_op, struct pxd_context, fops);
    pr_info("testmod_vm_close %p", file);
}

static void testmod_vm_open(struct vm_area_struct *vma)
{
	// struct file *file = vma->vm_file;
    pr_info("testmod_vm_open off %lx start %lx end %lx",
        vma->vm_pgoff << PAGE_SHIFT, vma->vm_start, vma->vm_end);

}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static vm_fault_t testmod_vm_fault(struct vm_fault *vmf)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5,1,0) && !defined(__EL8__)
static int testmod_vm_fault(struct vm_fault *vmf)
#else
static vm_fault_t testmod_vm_fault(struct vm_fault *vmf)
#endif
{
	int err;
	struct vm_area_struct *vma = vmf->vma;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
    // struct file *file = vma->vm_file;
#else
    // struct file *file = vmf->vma->vm_file;
#endif
    unsigned idx = (vmf->address - vma->vm_start) >> PAGE_SHIFT;
	    //((vma->vm_pgoff << PAGE_SHIFT) - vma->vm_start) >> PAGE_SHIFT;
    unsigned long start = (vma->vm_pgoff << PAGE_SHIFT) + vma->vm_start;

    unsigned long pfn;
    struct page* pg;

    err = getpfn(idx, &pfn);
    if (err) {
	    pr_err("vm fault index %x finding matching pfn failed", idx);
	    return -EFAULT;
    }

    pg = getpage(idx);
    pfn = page_to_pfn(pg);

    pr_info("mmap fault @%lx, vma->vm_pgoff %lu, idx %u, page %p, pfn %lu", vmf->address, vma->vm_pgoff, idx, pg, pfn);

    if (vmf->address < vma->vm_start || vmf->address >= vma->vm_end) {
	    pr_err("fault address outside range %lx <%lx, %lx>", vmf->address, vma->vm_start, vma->vm_end);
	    return -EFAULT;
    }

    //if ((vmf->pgoff << PAGE_SHIFT) > sizeof(struct fuse_conn_queues)) {
    // if (err != 0) 
    if (!pg) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,1,0)
        return -EFAULT;
#else
        return VM_FAULT_SIGSEGV;
#endif
    }

	zap_vma_ptes(vma, start, PAGE_SIZE);

    err = remap_pfn_range(vma, start, pfn, PAGE_SIZE, vma->vm_page_prot);
	//err =  vmf_insert_page(vma, vmf->address, pg);
	pr_info("remap page failed with error %d", err);

    // page = vmalloc_to_page(map_addr);
    // get_page(page);
    vmf->page = pg;
    return err;
}

static struct vm_operations_struct testmod_vm_ops = {
	.close = testmod_vm_close,
	.fault = testmod_vm_fault,
	.open = testmod_vm_open,
};

static int testmod_mmap(struct file *f, struct vm_area_struct *vma)
{
	// int i;
	unsigned long start;

	pr_info("inside mmap");
	testmod_vm_open(vma);

	start = vma->vm_start;
	// Ensure the area is large enough for the 16 page window
	if ((vma->vm_end - vma->vm_start) / PAGE_SIZE != 16) {
		pr_info("mmap fail, not enough mapped window");
		return -1;
	}

	vma->vm_ops = &testmod_vm_ops;
	// vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_IO | VM_PFNMAP;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP | VM_PFNMAP | VM_MIXEDMAP;
	// vma->vm_private_data = filp->private_data;
	//
#if 1
{
	int i;
	unsigned long pfn;
	// struct page *addr;

	for (i=0; i<16; i++) {
		int err = getpfn(i, &pfn);
		if (err != 0) return -1;
		// addr = getpage(i);
		// if (!addr) return -ENOMEM;

		pr_info("zap_vma_ptes for addr %lx", start);
		zap_vma_ptes(vma, start, PAGE_SIZE);

		// pfn = virt_to_phys(addr) >> PAGE_SHIFT;
		// pfn = virt_to_phys(addr) >> PAGE_SHIFT;
		pr_info("remap_pfn_range uaddr %lx, pfn %lx", start, pfn);
		err = remap_pfn_range(vma, start, pfn, PAGE_SIZE, vma->vm_page_prot);
		// err = vmf_insert_pfn(vma, start, pfn);
		// err = vm_insert_page(vma, start, addr);
		pr_info("remap page failed with error %d", err);
		if (err != 0) return -1;

		start += PAGE_SIZE;
	}
}
#endif
	pr_info("done mmap");
	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.read_iter = testmod_read_iter,
	.write_iter = testmod_write_iter,
	.unlocked_ioctl = testmod_ioctl,
	.mmap = testmod_mmap,
};

struct testcontext {
	struct miscdevice misc;
	bool ok;
};

static int testctxt_init(struct testcontext *tctx)
{
	int err;
	tctx->misc.minor = MISC_DYNAMIC_MINOR;
	tctx->misc.name = "testcontrol";
	tctx->misc.fops = &fops;

	pr_info("kmodule testctxt_init entry");
	if (kmemscratch_init()) {
		printk("scratchpad init failed\n");
		return -ENOMEM;
	}

	err =  misc_register(&tctx->misc);
	if (err) {
		printk(KERN_ERR "testctxt: failed to register misc device %s: err %d\n",
				tctx->misc.name, err);
	}
	tctx->ok = (err == 0);

	return err;
}

static void testctxt_exit(struct testcontext *tctx)
{
	pr_info("kmodule testctxt_exit entry");
	if (tctx->ok) {
		misc_deregister(&tctx->misc);
	}

	kmemscratch_exit();
}

static struct testcontext tctxt;

int testmod_entry(void)
{
	return testctxt_init(&tctxt);
}

void testmod_exit(void)
{
	testctxt_exit(&tctxt);
}

module_init(testmod_entry);
module_exit(testmod_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
