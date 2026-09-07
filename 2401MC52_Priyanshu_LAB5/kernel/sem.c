// kernel/sem.c — Counting semaphores using the kernel's sleep/wakeup primitives.
// Provides blocking synchronization for user programs without busy waiting.
// Semaphores are global and identified by small integer IDs.
//
// Syscalls:
//   sem_init(id, value) — initialize a semaphore.
//   sem_wait(id)        — decrement or block when unavailable.
//   sem_signal(id)      — increment and wake a waiting process.

 
#include "types.h"
#include "riscv.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
 
#define NSEM 16   // how many independent semaphores the kernel supports
 
struct semaphore {
  struct spinlock lock;  // protects `value`; also the sleep/wakeup lock
  int value;
  int inited;
};
 
static struct semaphore semtable[NSEM];
 
// Call once at boot, from main.c.
void
seminit(void)
{
  for(int i = 0; i < NSEM; i++){
    initlock(&semtable[i].lock, "sem");
    semtable[i].value = 0;
    semtable[i].inited = 0;
  }
}
 
// (Re-)initialize semaphore `id` to `value`. Must be called before the
// first sem_wait()/sem_signal() on that id (typically by the parent,
// before it forks).
int
sem_init(int id, int value)
{
  if(id < 0 || id >= NSEM)
    return -1;
  acquire(&semtable[id].lock);
  semtable[id].value = value;
  semtable[id].inited = 1;
  release(&semtable[id].lock);
  return 0;
}
 
// P(id) / down(id): decrement the semaphore or block if unavailable.
// The while-loop rechecks the condition after wakeup to handle
// multiple waiters or spurious wakeups.
//
// sleep_prepare(chan) marks the process as a waiter while holding
// the semaphore lock; sleep() then blocks if it was not already woken.
// Safe pattern: prepare -> release lock -> sleep -> reacquire -> recheck.
// This prevents lost wakeups.

int
sem_wait(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;
  struct semaphore *s = &semtable[id];
 
  acquire(&s->lock);
  while(s->value <= 0){
    sleep_prepare(s);     // register intent to sleep on chan = s
    release(&s->lock);    // safe to release now; wakeup() can't be missed
    sleep();               // actually block (no-op if already woken)
    acquire(&s->lock);     // re-acquire before rechecking the condition
  }
  s->value--;
  release(&s->lock);
  return 0;
}
 
// V(id) / up(id): increment the semaphore and wake up anyone waiting
// on it. wakeup() is called after releasing s->lock, which is safe
// here (wakeup/sleep_prepare synchronize via each process's own
// p->lock) and avoids holding s->lock while scanning the whole process
// table.
int
sem_signal(int id)
{
  if(id < 0 || id >= NSEM)
    return -1;
  struct semaphore *s = &semtable[id];
 
  acquire(&s->lock);
  s->value++;
  release(&s->lock);
  wakeup(s);
  return 0;
}
 
