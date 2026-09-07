// user/peterson.c  --  NEW FILE
//
// Question 1: Mutual exclusion between a parent and a child process
// using Peterson's Algorithm, on a page of memory obtained through the
// new shm_get() syscall (see kernel/shm.c).
//
// Process 0 = parent, Process 1 = child. Each does ITERS iterations of:
//   entry section (Peterson's protocol)  ->  critical section (increment
//   shared_counter and print)  ->  exit section  ->  remainder section
//   (a short delay, so the two processes actually race for the CS).
//
// No OS lock is used to enforce the mutual exclusion itself -- only the
// flag[]/turn variables in shared memory, exactly as Peterson's
// Algorithm specifies. __sync_synchronize() memory barriers are used
// around the shared-variable accesses so the protocol is correct even
// if the parent and child happen to run truly in parallel on different
// cores (xv6-riscv normally runs with multiple harts / CPUs).

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

struct shared {
  int flag[2];
  int turn;
  int shared_counter;
};

#define ITERS 10

static void
delay(int n)
{
  for(volatile int i = 0; i < n; i++)
    ;
}

int
main(int argc, char *argv[])
{
  uint64 va = shm_get();
  if(va == 0xffffffffffffffff){
    printf("peterson: shm_get failed\n");
    exit(1);
  }
  struct shared *s = (struct shared *)va;

  // We are the parent at this point (this call happens before fork()),
  // so it's safe to initialize the shared state here.
  s->flag[0] = 0;
  s->flag[1] = 0;
  s->turn = 0;
  s->shared_counter = 0;

  int pid = fork();
  int id, other;

  if(pid < 0){
    printf("peterson: fork failed\n");
    exit(1);
  } else if(pid == 0){
    // Child: map the SAME shared page (already created by the parent)
    // into our own address space.
    id = 1;
    other = 0;
    va = shm_get();
    if(va == 0xffffffffffffffff){
      printf("peterson: child shm_get failed\n");
      exit(1);
    }
    s = (struct shared *)va;
  } else {
    id = 0;
    other = 1;
  }

  for(int i = 0; i < ITERS; i++){
    // ---- entry section (Peterson's Algorithm) ----
    s->flag[id] = 1;
    s->turn = other;
    __sync_synchronize();
    while(s->flag[other] == 1 && s->turn == other)
      ;  // busy-wait

    // ---- critical section ----
    s->shared_counter = s->shared_counter + 1;
    printf("Process %d in CS, counter = %d\n", id, s->shared_counter);

    // ---- exit section ----
    __sync_synchronize();
    s->flag[id] = 0;

    // ---- remainder section ----
    delay(100000 + 37 * id);
  }

  if(pid > 0){
    wait(0);
    printf("Final shared_counter = %d (expected %d)\n", s->shared_counter, 2 * ITERS);
  }

  exit(0);
}