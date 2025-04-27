/*
 * Copyright (C) 2025 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Sierra release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
// #define MM_PAGING
/*
 * System Library
 * Memory Module Library libmem.c
 */

 #include "string.h"
 #include "mm.h"
 #include "syscall.h"
 #include "libmem.h"
 #include <stdlib.h>
 #include <stdio.h>
 #include <pthread.h>
 
 static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
 
 /*enlist_vm_freerg_list - add new rg to freerg_list
  *@mm: memory region
  *@rg_elmt: new region
  *
  */
 
 
 int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
 {
   struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
   if (rg_elmt->rg_start >= rg_elmt->rg_end)
     return -1;
   if (rg_node != NULL)
     rg_elmt->rg_next = rg_node;
   mm->mmap->vm_freerg_list = rg_elmt;
   return 0;
 }
 
 struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
 {
   if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
     return NULL;
   return &mm->symrgtbl[rgid];
 }
 
 /*__alloc - allocate a region memory
  *@caller: caller
  *@vmaid: ID vm area to alloc memory region
  *@rgid: memory region ID (used to identify variable in symbole table)
  *@size: allocated size
  *@alloc_addr: address of allocated memory region
  *
  */
 int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
 {
   
   /*Allocate at the toproof */
   struct vm_rg_struct rgnode;
 
   /* TODO: commit the vmaid */
   // rgnode.vmaid
 
   /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
   if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
   {
    pthread_mutex_lock(&mmvm_lock);
     caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
     caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
     *alloc_addr = rgnode.rg_start;
     // printf("rgnode.rg_start = %ld\n", rgnode.rg_start);
     // printf("rgnode.rg_end = %ld\n", rgnode.rg_end);
     pthread_mutex_unlock(&mmvm_lock);
     return 0;
   }
 
   printf("rgnode.rg_start = %ld\n", rgnode.rg_start);
   printf("rgnode.rg_end = %ld\n", rgnode.rg_end);
 
   /* TODO retrive current vma if needed, current comment out due to compiler redundant warning*/
   /*Attempt to increate limit to get space */
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   // int inc_sz = PAGING_PAGE_ALIGNSZ(size);
   // TODO: 16/4/2025
   int inc_sz = (size);
 
   /* TODO retrive old_sbrk if needed, current comment out due to compiler redundant warning*/
   int old_sbrk = cur_vma->sbrk;
   printf("old_sbrk = %d\n", old_sbrk);
   printf("inc_sz = %d\n", inc_sz);
 
   /* TODO INCREASE THE LIMIT as invoking systemcall
    * sys_memap with SYSMEM_INC_OP
    */
   struct sc_regs regs;
   regs.a1 = SYSMEM_INC_OP;
   regs.a2 = vmaid;
   regs.a3 = inc_sz; // size mở rộng
 
   /* SYSCALL 17 sys_memmap */
   regs.orig_ax = 17;
   if (syscall(caller, regs.orig_ax, &regs) != 0) // nếu mở rộng không thành công
   {
     regs.flags = -1;                    // failed
     perror("failed to increase limit"); // in syscall
     pthread_mutex_unlock(&mmvm_lock);   //
     return -1;
   }
   regs.flags = 0; // successful
 
   /* TODO: commit the limit increment */
   caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
   caller->mm->symrgtbl[rgid].rg_end = old_sbrk + PAGING_PAGE_ALIGNSZ(size);
 
   printf("caller->mm->symrgtbl[rgid].rg_start = %ld\n", caller->mm->symrgtbl[rgid].rg_start);
   printf("caller->mm->symrgtbl[rgid].rg_end = %ld\n", caller->mm->symrgtbl[rgid].rg_end);
 
   printf("cur_vma->vm_start = %ld\n", cur_vma->vm_start);
   printf("cur_vma->vm_end = %ld\n", cur_vma->vm_end);
 
   /* TODO: commit the allocation address */
   *alloc_addr = old_sbrk;
 
   if (old_sbrk + size < cur_vma->vm_end)
   {
     struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));
     rgnode->rg_start = old_sbrk + size;
     rgnode->rg_end = cur_vma->vm_end;
 
     enlist_vm_freerg_list(caller->mm, rgnode);
   }
 
   pthread_mutex_unlock(&mmvm_lock);
   return 0;
 }
 
 int __free(struct pcb_t *caller, int vmaid, int rgid)
 {
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
  return -1;
struct vm_rg_struct *rgnode=get_symrg_byid(caller->mm,rgid);
/* TODO: Manage the collect freed region to freerg_list */
struct vm_rg_struct *freerg = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));
freerg->rg_start = rgnode->rg_start;
freerg->rg_end = rgnode->rg_end;
freerg->rg_next = NULL;

/*enlist the obsoleted memory region */
enlist_vm_freerg_list(caller->mm,freerg);
return 0;
 }
 int liballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
 {
   int addr;
   int val = __alloc(proc, 0, reg_index, size, &addr);
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER ALLOCATION =====\n");
   // printf("PID=%d - Region=%d - Address=%08ld - Size=%d byte\n", proc->pid, reg_index, addr * sizeof(uint32_t), size);
   printf("PID=%d - Region=%d - Address=%08X - Size=%d byte\n", proc->pid, reg_index, addr, size);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
   for (int i = 0; i < PAGING_MAX_PGN; i++)
   {
     uint32_t pte = proc->mm->pgd[i];
     if (PAGING_PAGE_PRESENT(pte))
     {
       int fpn = PAGING_PTE_FPN(pte);
       printf("Page Number: %d -> Frame Number: %d\n", i, fpn);
     }
   }
   printf("================================================================\n");
 #endif
   return val;
 }
 int libfree(struct pcb_t *proc, uint32_t reg_index)
 {
   int val = __free(proc, 0, reg_index);
 #ifdef IODUMP
   printf("===== PHYSICAL MEMORY AFTER DEALLOCATION =====\n");
   printf("PID=%d - Region=%d\n", proc->pid, reg_index);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1);
 #endif
   MEMPHY_dump(proc->mram);
   for (int i = 0; i < PAGING_MAX_PGN; i++)
   {
     uint32_t pte = proc->mm->pgd[i];
     if (PAGING_PAGE_PRESENT(pte))
     {
       int fpn = PAGING_PTE_FPN(pte);
       printf("Page Number: %d -> Frame Number: %d\n", i, fpn);
     }
   }
   printf("================================================================\n");
 #endif
   return val;
 }
 int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
 {
   uint32_t pte = mm->pgd[pgn];
   if (!PAGING_PAGE_PRESENT(pte))
   {
     int new_fpn;
     if (MEMPHY_get_freefp(caller->mram, &new_fpn) < 0)
     {
       int vicpgn;
       find_victim_page(caller->mm, &vicpgn);
       uint32_t vic_pte = mm->pgd[vicpgn];
       int vicfpn = PAGING_FPN(vic_pte);
       int swpfpn;
       MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
       __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
       pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);
       new_fpn = vicfpn;
     }
     pte_set_fpn(&mm->pgd[pgn], new_fpn);
   }
   else if (pte & PAGING_PTE_SWAPPED_MASK)
   {
     int vicpgn, swpfpn;
     int tgtfpn = PAGING_SWP(pte);
     find_victim_page(caller->mm, &vicpgn);
     uint32_t vic_pte = mm->pgd[vicpgn];
     int vicfpn = PAGING_FPN(vic_pte);
     MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
     __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
     __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);
     pte_set_swap(&mm->pgd[vicpgn], 0, swpfpn);
     pte_set_fpn(&mm->pgd[pgn], vicfpn);
     enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
   }
   *fpn = PAGING_FPN(mm->pgd[pgn]);
   return 0;
 }
 int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1;
   int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
   MEMPHY_read(caller->mram, phyaddr, data);
   return 0;
 }
 int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
 {
   int pgn = PAGING_PGN(addr);
   int off = PAGING_OFFST(addr);
   int fpn;
   if (pg_getpage(mm, pgn, &fpn, caller) != 0)
     return -1;
   int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
   MEMPHY_write(caller->mram, phyaddr, value);
   return 0;
 }
 int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
 {
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
     return -1;
   pg_getval(caller->mm, currg->rg_start + offset, data, caller);
   return *data;
 }
 int libread(
     struct pcb_t *proc, // Process executing the instruction
     uint32_t source,    // Index of source register
     uint32_t offset,    // Source address = [source] + [offset]
     uint32_t *destination)
 {
   BYTE data;
   int val = __read(proc, 0, source, offset, &data);
   *destination = (uint32_t)data;
 #ifdef IODUMP
   printf("================================================================\n");
   printf("===== PHYSICAL MEMORY AFTER READING =====\n");
   printf("read region=%d offset=%d value=%d\n", source, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
 #endif
 
   return val;
 }
 int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
 {
   pthread_mutex_lock(&mmvm_lock);
   struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   if (currg == NULL || cur_vma == NULL)
   {
     pthread_mutex_unlock(&mmvm_lock);
     return -1;
   }
   pg_setval(caller->mm, currg->rg_start + offset, value, caller);
   pthread_mutex_unlock(&mmvm_lock);
   return 0;
 }
 int libwrite(
     struct pcb_t *proc,   // Process executing the instruction
     BYTE data,            // Data to be wrttien into memory
     uint32_t destination, // Index of destination register
     uint32_t offset)
 {
 #ifdef IODUMP
   printf("================================================================\n");
   printf("===== PHYSICAL MEMORY AFTER WRITING =====\n");
   printf("write region=%d offset=%d value=%d\n", destination, offset, data);
 #ifdef PAGETBL_DUMP
   print_pgtbl(proc, 0, -1); // print max TBL
 #endif
   MEMPHY_dump(proc->mram);
   for (int i = 0; i < PAGING_MAX_PGN; i++)
   {
     uint32_t pte = proc->mm->pgd[i];
     if (PAGING_PAGE_PRESENT(pte))
     {
       int fpn = PAGING_PTE_FPN(pte);
       printf("Page Number: %d -> Frame Number: %d\n", i, fpn);
     }
   }
   printf("================================================================\n");
 #endif
   return __write(proc, 0, destination, offset, data);
 }
 int free_pcb_memph(struct pcb_t *caller)
 {
   int pagenum, fpn;
   uint32_t pte;
   for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
   {
     pte = caller->mm->pgd[pagenum];
     if (!PAGING_PAGE_PRESENT(pte))
     {
       fpn = PAGING_PTE_FPN(pte);
       MEMPHY_put_freefp(caller->mram, fpn);
     }
     else
     {
       fpn = PAGING_PTE_SWP(pte);
       MEMPHY_put_freefp(caller->active_mswp, fpn);
     }
   }
   return 0;
 }
 int find_victim_page(struct mm_struct *mm, int *retpgn)
 {
   struct pgn_t *pg = mm->fifo_pgn;
   if (!pg)
     return -1;
   while (pg->pg_next && pg->pg_next->pg_next)
     pg = pg->pg_next;
   if (pg->pg_next)
   {
     *retpgn = pg->pg_next->pgn;
     free(pg->pg_next);
     pg->pg_next = NULL;
   }
   else
   {
     *retpgn = pg->pgn;
     free(pg);
     mm->fifo_pgn = NULL;
   }
   return 0;
 }
 int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
 {
   struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
   struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
   if (rgit == NULL)
     return -1;
   newrg->rg_start = newrg->rg_end = -1;
   while (rgit != NULL)
   {
     if (rgit->rg_start + size <= rgit->rg_end)
     {
       newrg->rg_start = rgit->rg_start;
       newrg->rg_end = rgit->rg_start + size;
       if (rgit->rg_start + size < rgit->rg_end)
       {
         rgit->rg_start = rgit->rg_start + size;
       }
       else
       {
         struct vm_rg_struct *nextrg = rgit->rg_next;
         if (nextrg != NULL)
         {
           rgit->rg_start = nextrg->rg_start;
           rgit->rg_end = nextrg->rg_end;
 
           rgit->rg_next = nextrg->rg_next;
 
           free(nextrg);
         }
         else
         {
           rgit->rg_start = rgit->rg_end;
           rgit->rg_next = NULL;
         }
       }
     }
     else
     {
       rgit = rgit->rg_next;
     }
   }
   if (newrg->rg_start == -1)
     return -1;
   return 0;
 }
 