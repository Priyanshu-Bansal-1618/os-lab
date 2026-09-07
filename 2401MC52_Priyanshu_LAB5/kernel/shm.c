// kernel/shm.c  --  NEW FILE
//
// Minimal shared-memory facility for xv6-riscv.
//
// Provides one syscall, shm_get(), which:
//   - The first time it is called by ANY process, allocates a single
//     physical page (via kalloc()) and maps it into that process's
//     address space at a fixed virtual address, SHMVA.
//   - The next time it is called (typically by a child created with
//     fork() after the parent already called shm_get()), it maps the
//     SAME physical page into the caller's own page table at SHMVA.
//   - Returns SHMVA on success, or (uint64)-1 on failure.
//
// This gives a parent and its child a page of memory that both of them
// can read and write directly -- exactly what Peterson's Algorithm
// needs, since xv6 processes otherwise have completely private address
// spaces (fork() copies memory, it does not share it).
//
// The page is reference counted (shm_refcnt) so it is only freed once
// every process that mapped it has exited (see shm_unmap(), which is
// called from freeproc() in proc.c).
//
// NOTE: this implementation intentionally supports a single shared page
// system-wide, which is all Assignment 5 / Question 1 requires (one
// parent + one child). If you need multiple independent shared regions
// at once, generalize shm_page/shm_refcnt into a small table indexed by
// a "shm id" and change SHMVA into an array of fixed VAs.

#include "types.h"
#include "riscv.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

static struct spinlock shm_lock;
static char *shm_page = 0;    // the single shared physical page (0 = not yet allocated)
static int shm_refcnt = 0;    // number of processes that currently have it mapped

// Call once at boot, from main.c.
void
shminit(void)
{
  initlock(&shm_lock, "shm");
}

// Implements the shm_get() syscall body (called from sys_shm_get()).
uint64
shm_get(void)
{
  struct proc *p = myproc();

  // Already mapped in this process? Just hand back the same VA.
  if(p->shm_mapped)
    return SHMVA;

  acquire(&shm_lock);
  if(shm_page == 0){
    // Nobody has created the shared page yet -- allocate it.
    release(&shm_lock);
    char *mem = kalloc();
    if(mem == 0)
      return (uint64)-1;
    memset(mem, 0, PGSIZE);

    acquire(&shm_lock);
    if(shm_page == 0){
      shm_page = mem;
    } else {
      // Lost a race with another process allocating first; drop ours.
      release(&shm_lock);
      kfree(mem);
      acquire(&shm_lock);
    }
  }
  char *pa = shm_page;
  release(&shm_lock);

  // Map the existing physical page into the caller's own page table.
  if(mappages(p->pagetable, SHMVA, PGSIZE, (uint64)pa, PTE_R | PTE_W | PTE_U) < 0)
    return (uint64)-1;

  acquire(&shm_lock);
  shm_refcnt++;
  release(&shm_lock);

  p->shm_mapped = 1;
  return SHMVA;
}

// Called from freeproc() when a process that had mapped the shared page
// exits. Unmaps SHMVA from that process's page table WITHOUT freeing the
// physical page (the other process may still be using it), then frees
// the physical page once the reference count hits zero.
void
shm_unmap(struct proc *p)
{
  if(!p->shm_mapped)
    return;

  uvmunmap(p->pagetable, SHMVA, 1, 0);  // do_free = 0: keep the physical page

  acquire(&shm_lock);
  shm_refcnt--;
  if(shm_refcnt <= 0 && shm_page != 0){
    kfree(shm_page);
    shm_page = 0;
    shm_refcnt = 0;
  }
  release(&shm_lock);

  p->shm_mapped = 0;
}
