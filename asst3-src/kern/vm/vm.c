#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <proc.h>
#include <current.h>
#include <spl.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */
vaddr_t fhead = 0;
uint32_t hpt_size = 0;
struct frame_table_entry *ftable = 0;
struct page_table_entry *ptable = 0;

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr);

void vm_bootstrap(void) {

	/* init frame table */
	fhead = PADDR_TO_KVADDR(ram_getfirstfree()); /* top of kernel */
	ftable = (struct frame_table_entry *) fhead;
	paddr_t size = ram_getsize();
	for (uint32_t i = 0; i < size; ++i) {
		(ftable + i)->addr = i >> PAGE_BITS;
		if (i == size - 1) {
			(ftable + i)->next = 0; /* circular linked list */
		} else {
			(ftable + i)->next = (i + 1) >> PAGE_BITS;
		}
	}

	/* set first free frame to be the frame immediately after ftable */
	fhead += size;

	/* init page table */
	hpt_size = size * 2;
	ptable = (struct page_table_entry *) fhead;
	fhead += hpt_size;
	for (uint32_t i = 0; i < hpt_size; ++i) {
		/* only need to ensure all next ptrs are null */
		(ptable + i)->next = NULL;
	}
}

uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr) {
	uint32_t index;
	index = (((uint32_t) as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
	return index;
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
	switch (faulttype) {
	case VM_FAULT_READONLY:
		/* TODO: handle later */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break;
	default:
		return EINVAL;
	}

	struct addrspace *as = proc_getas();
	if (curproc == NULL) return EFAULT;
	if (as == NULL) return EFAULT;

	pid_t pid = (uint32_t) as;
	faultaddress &= PAGE_FRAME;
	uint32_t index = hpt_hash(as, faultaddress);
	if (((ptable + index)->entryhi & TLBHI_VPAGE) >> PAGE_BITS == faultaddress && pid == (ptable + index)->pid) { /* TODO: may not check pid here */
		int spl = splhigh();
		tlb_random((ptable + index)->entryhi, (ptable + index)->entrylo);
		splx(spl);
		return 0;
	} else {
		ptable_entry curr = &ptable[index];
		while (curr->next != NULL) {
			if ((curr->entryhi & TLBHI_VPAGE) >> PAGE_BITS == faultaddress && pid == curr->pid) break; /* TODO: see above */
			curr = curr->next;
		}
		if (curr == NULL) {
			return EFAULT;
		} else {
			int spl = splhigh();
			tlb_random(curr->entryhi, curr->entrylo);
			splx(spl);
		}
		return 0;
	}

	return EFAULT;
}

/*
 * SMP-specific functions. Unused in our configuration.
 */
void vm_tlbshootdown(const struct tlbshootdown *ts) {
	(void) ts;
	panic("vm tried to do tlb shootdown?!\n");
}
