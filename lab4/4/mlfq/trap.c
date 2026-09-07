#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    /* MLFQ time quanta: level 0 = 1 tick, level 1 = 2 ticks,
     * level 2 = 4 ticks.  A process that consumes its complete quantum
     * is demoted and yields the CPU. */
    if(myproc() != 0 && myproc()->state == RUNNING){
      int quantum;

      if(myproc()->queue_level == 0)
        quantum = 1;
      else if(myproc()->queue_level == 1)
        quantum = 2;
      else
        quantum = 4;

      myproc()->ticks_used++;

      if(myproc()->ticks_used >= quantum){
        if(myproc()->queue_level < 2){
          myproc()->queue_level++;
          cprintf("MLFQ: pid %d demoted to queue %d\n",
                  myproc()->pid, myproc()->queue_level);
        }
        myproc()->ticks_used = 0;

        /* Only preempt a user process at a quantum boundary. */
        if((tf->cs&3) == DPL_USER)
          yield();
      }
    }
    break;

  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
    // Bochs generates spurious IRQ7.
    break;

  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    cprintf("pid %d %s: trap %d err %d on cpu %d eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

}
