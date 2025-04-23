#include "common.h"
#include "syscall.h"
#include "stdio.h"
#include "queue.h"

int sys_killallhandler(struct pcb_t *caller, struct sc_regs *regs)
{
    int region_id = regs->a1;
    char program_name[100];
    uint32_t addr;
    addr = (uint32_t)get_region_by_id(caller->mm, region_id);
    
    if (addr == 0) {
        return -1;
    }
    int i = 0;
    char c;
    do {
        MEMPHY_read(caller->mram, addr + i, &c);
        program_name[i++] = c;
    } while (c != '\0' && i < 99);
    program_name[i] = '\0';
    printf("Killing all processes with name: %s\n", program_name);
    int killed_count = 0;
    for (int prio = 0; prio < MAX_PRIO; prio++) {
        struct queue_t *q = get_mlq_by_priority(prio);
        if (q == NULL) continue;
        struct queue_t temp_queue;
        queue_init(&temp_queue);
        struct pcb_t *proc;
        while ((proc = dequeue(q)) != NULL) {
            if (strcmp(proc->path, program_name) == 0) {
                printf("Killing process %d with name %s\n", proc->pid, proc->path);
                cleanup_process(proc);  
                killed_count++;
            } else {
                enqueue(&temp_queue, proc);
            }
        }
        while ((proc = dequeue(&temp_queue)) != NULL) {
            enqueue(q, proc);
        }
    }
    printf("Killed %d processes with name %s\n", killed_count, program_name);
    return killed_count;
}
void cleanup_process(struct pcb_t *proc) {
    if (proc->mm != NULL) {
        struct vm_area_struct *vma = proc->mm->mmap;
        while (vma != NULL) {
            struct vm_area_struct *next = vma->vm_next;
            struct vm_rg_struct *rg = vma->vm_freerg_list;
            while (rg != NULL) {
                struct vm_rg_struct *next_rg = rg->rg_next;
                free(rg);
                rg = next_rg;
            }
            free(vma);
            vma = next;
        }
        if (proc->mm->pgd != NULL) {
            free(proc->mm->pgd);
        }
        free(proc->mm);
    }
    free(proc);
}