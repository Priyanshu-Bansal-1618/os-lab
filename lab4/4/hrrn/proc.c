#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

/* Return the index of the CPU that is currently running. */
int
cpuid() {
  return mycpu()-cpus;
}

/* Find the CPU structure for the current processor. */
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

/* Return the process currently running on this CPU. */
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

/* Find an unused process entry and prepare it for use. */
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  /* Save the creation time used by the HRRN scheduler. */
  acquire(&tickslock);
  p->ctime = ticks;
  release(&tickslock);
  p->burst_time = 1;

  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

/* Create and initialize the first user process. */
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

/* Increase or decrease the memory size of the current process. */
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

/* Create a child process by copying the current process. */
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  if((np = allocproc()) == 0){
    return -1;
  }

  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

/* Finish the current process and mark it as a zombie. */
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  wakeup1(curproc->parent);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

/* Wait for a child process to finish and clean it up. */
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    sleep(curproc, &ptable.lock); 
  }
}

/*
 * Choose the RUNNABLE process with the highest HRRN response ratio.
 *
 * Response Ratio = ((Wait Time + Burst Time) * 100) / Burst Time
 * where Wait Time = current ticks - creation time.
 *
 * HRRN is non-preemptive: once a process is selected, it keeps the CPU
 * until it blocks, yields voluntarily, or exits.  trap.c therefore does
 * not perform timer-based preemption for this scheduler.
 */
void
scheduler(void)
{
  struct proc *p;
  struct proc *selected;
  struct cpu *c = mycpu();
  uint now;
  uint wait_time;
  uint ratio;
  uint selected_wait;
  uint selected_ratio;
  c->proc = 0;

  for(;;){
    sti();

    acquire(&ptable.lock);

    selected = 0;
    acquire(&tickslock);
    now = ticks;
    release(&tickslock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

      if(p->state != RUNNABLE)
        continue;

      /* A default burst of 1 is used until set_burst() supplies the
       * process's expected burst. */
      if(p->burst_time <= 0)
        continue;

      wait_time = now - p->ctime;
      ratio = ((wait_time + p->burst_time) * 100) / p->burst_time;

      if(selected == 0){
        selected = p;
      } else {
        selected_wait = now - selected->ctime;
        selected_ratio =
            ((selected_wait + selected->burst_time) * 100) / selected->burst_time;

        if(ratio > selected_ratio)
          selected = p;
      }
    }

    if(selected != 0){
      p = selected;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      /* Non-preemptive HRRN: the process runs until it blocks, exits,
       * or voluntarily yields. */
      swtch(&(c->scheduler), p->context);
      switchkvm();

      c->proc = 0;
    }

    release(&ptable.lock);
  }
}

/* Set the expected CPU burst time used by the HRRN scheduler. */
int
set_burst(int pid, int bt)
{
  struct proc *p;

  if(bt <= 0)
    return -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid && p->state != UNUSED){
      p->burst_time = bt;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

/* Switch from the current process back to the scheduler. */
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

/* Give up the CPU and allow the scheduler to choose again. */
void
yield(void)
{
  acquire(&ptable.lock);  
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

/* Complete the first setup performed when a child starts running. */
void
forkret(void)
{
  static int first = 1;
  
  release(&ptable.lock);

  if (first) {

    
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

}

/* Put the current process to sleep on a channel. */
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  

  
  
  if(lk != &ptable.lock){  
    acquire(&ptable.lock);  
    release(lk);
  }
  
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  p->chan = 0;

  if(lk != &ptable.lock){  
    release(&ptable.lock);
    acquire(lk);
  }
}

/* Make sleeping processes on the given channel runnable. */
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

/* Safely wake all processes waiting on a channel. */
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

/* Mark the selected process as killed. */
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

/* Print the current process table for debugging. */
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
