#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

/* Place your frametable data-structures here
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */
vaddr_t fhead;
struct frame_table_entry *ftable;
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

/*
 * Note that this function returns a VIRTUAL address, not a physical
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap(). You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */
vaddr_t alloc_kpages(unsigned int npages) {
	if (npages != 1) {
		kprintf("%s: called with npages != 1!\n", __func__);
		return 0;
	}

	paddr_t addr;

	spinlock_acquire(&stealmem_lock);
	if (ftable == 0) {
		kprintf("%s: using ram_stealmem instead\n", __func__);
		addr = ram_stealmem(npages);
	} else {
		kprintf("%s: using the frame table allocator\n", __func__);
		addr = (paddr_t)(((struct frame_table_entry *) fhead)->addr);
		fhead = (vaddr_t)(((struct frame_table_entry *) fhead)->next);
		kprintf("%s: address is %u (kernel virtual: %u)\n", __func__, addr, PADDR_TO_KVADDR(addr));
	}
	spinlock_release(&stealmem_lock);

	if (addr == 0) return 0;

	/* TODO: zero out the new frame */

	return PADDR_TO_KVADDR(addr);
}

void free_kpages(vaddr_t addr) {
	spinlock_acquire(&stealmem_lock);
	vaddr_t old_head = fhead;
	fhead = KVADDR_TO_PADDR(addr);
	((struct frame_table_entry *) fhead)->next = old_head;
	spinlock_release(&stealmem_lock);
}
