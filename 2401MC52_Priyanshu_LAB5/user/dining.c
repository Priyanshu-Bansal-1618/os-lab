// user/dining.c  --  NEW FILE
//
// Question 4: Dining Philosophers Problem, 5 philosophers / 5 forks,
// deadlock-free by construction.
//
// Reuses sem_init/sem_wait/sem_signal from Question 2 (kernel/sem.c):
// each fork is a binary semaphore (id = fork number, 0..4). No new
// kernel code and no shared memory are needed for this question --
// forks are entirely represented by kernel-tracked semaphores, and
// each philosopher only needs to know its own id.
//
// Deadlock-avoidance strategy: RESOURCE ORDERING.
//   Philosopher i's "left" fork is i, "right" fork is (i+1) % 5.
//   If every philosopher always picked up their LEFT fork first, all 5
//   could each grab their left fork simultaneously and then block
//   forever waiting for their right fork -- a circular wait, which is
//   exactly how the classic deadlock happens.
//   Instead, every philosopher here picks up the LOWER-NUMBERED fork
//   first and the higher-numbered fork second, regardless of whether
//   that's their "left" or "right" fork. Because fork ids form a total
//   order (0 < 1 < 2 < 3 < 4), no philosopher can ever be holding a
//   higher-numbered fork while waiting on a lower-numbered one that
//   another philosopher already holds -- this breaks the circular-wait
//   condition (one of the four necessary conditions for deadlock), so
//   deadlock is provably impossible.
//   Concretely, this only changes the behavior of philosopher 4 (whose
//   "left"=4, "right"=0): everyone else picks up their numerically
//   lower left fork first as usual, but philosopher 4 picks up its
//   right fork (0) before its left fork (4) -- i.e. this reduces to the
//   well-known "one philosopher picks up forks in the opposite order"
//   (asymmetric) trick, derived here systematically from fork numbering
//   rather than special-cased by hand.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_PHIL       5
#define DEFAULT_CYCLES 5

static void
delay(int n)
{
  for(volatile int i = 0; i < n; i++)
    ;
}

static int
left_fork(int id)
{
  return id;
}

static int
right_fork(int id)
{
  return (id + 1) % NUM_PHIL;
}

static void
philosopher(int id, int cycles)
{
  int pid = getpid();
  int lf = left_fork(id);
  int rf = right_fork(id);

  // Resource ordering: always acquire the lower-numbered fork first.
  int first  = (lf < rf) ? lf : rf;
  int second = (lf < rf) ? rf : lf;

  for(int c = 0; c < cycles; c++){
    // ---- THINKING ----
    printf("Philosopher %d (pid %d): THINKING\n", id, pid);
    delay(200000 + 15000 * id);

    // ---- HUNGRY ----
    printf("Philosopher %d (pid %d): THINKING -> HUNGRY\n", id, pid);
    sem_wait(first);
    sem_wait(second);

    // ---- EATING ----
    printf("Philosopher %d (pid %d): HUNGRY -> EATING (forks %d & %d, cycle %d/%d)\n",
           id, pid, lf, rf, c + 1, cycles);
    delay(250000);

    sem_signal(second);
    sem_signal(first);

    // ---- back to THINKING ----
    printf("Philosopher %d (pid %d): EATING -> THINKING\n", id, pid);
  }

  printf("Philosopher %d (pid %d): completed all %d cycles\n", id, pid, cycles);
}

int
main(int argc, char *argv[])
{
  int cycles = DEFAULT_CYCLES;
  if(argc > 1){
    cycles = atoi(argv[1]);
    if(cycles < 1){
      printf("dining: cycles must be >= 1\n");
      exit(1);
    }
  }

  // One binary semaphore per fork, all starting "available" (1).
  for(int i = 0; i < NUM_PHIL; i++)
    sem_init(i, 1);

  printf("dining: starting %d philosophers, %d cycles each\n", NUM_PHIL, cycles);

  for(int i = 0; i < NUM_PHIL; i++){
    int pid = fork();
    if(pid < 0){
      printf("dining: fork failed\n");
      exit(1);
    } else if(pid == 0){
      philosopher(i, cycles);
      exit(0);
    }
    // parent loops around to fork the next philosopher
  }

  for(int i = 0; i < NUM_PHIL; i++)
    wait(0);

  printf("dining: all %d philosophers finished %d cycles each -- no deadlock occurred\n",
         NUM_PHIL, cycles);
  exit(0);
}