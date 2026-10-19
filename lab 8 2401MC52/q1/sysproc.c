#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_va2pa(void)
{
  int va;
  pte_t *pte;
  uint pa;
  struct proc *cur = myproc();

  if(argint(0, &va) < 0)
    return -1;
  pte = walkpgdir(cur->pgdir, (void*)va, 0);
  if(pte == 0 || !(*pte & PTE_P) || !(*pte & PTE_U)){
    cprintf("va2pa: va=%x -> not present or not user-accessible (return -1)\n", (uint)va);
    return -1;
  }
  pa = PTE_ADDR(*pte) + ((uint)va & 0xFFF);
  cprintf("va2pa: va=%x PDX=%d PTX=%d off=%x PTE=%x flags P=%d W=%d U=%d -> pa=%x\n",
          (uint)va, PDX(va), PTX(va), (uint)va & 0xFFF, (uint)*pte,
          (*pte & PTE_P) ? 1 : 0, (*pte & PTE_W) ? 1 : 0, (*pte & PTE_U) ? 1 : 0, pa);
  return (int)pa;
}
