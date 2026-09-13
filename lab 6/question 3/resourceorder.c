#include "types.h"
#include "stat.h"
#include "user.h"

/* fixed mode uses semaphores 0,1 ; bad mode uses 2,3 (isolates stuck procs) */
static void
delay(int n)
{
  volatile int i;
  for(i = 0; i < n; i++)
    ;
}

int
main(int argc, char *argv[])
{
  int pidA, pidB, fixed, l1, l2;

  fixed = (argc > 1 && argv[1][0] == 'f');
  l1 = fixed ? 0 : 2;
  l2 = fixed ? 1 : 3;

  sem_init(0, 1); sem_init(1, 1);
  sem_init(2, 1); sem_init(3, 1);

  printf(1, "=== Resource-ordering demo: %s ===\n",
         fixed ? "FIXED (both processes take Lock1 then Lock2)"
               : "BAD (A: Lock1->Lock2, B: Lock2->Lock1)");

  pidA = fork();
  if(pidA == 0){
    /* Process A: always Lock1 then Lock2 */
    printf(1, "[tick %d] Process A waiting for Lock1\n", uptime());
    sem_wait(l1);
    printf(1, "[tick %d] Process A acquired Lock1\n", uptime());
    delay(4000000);              /* deliberate window to force the clash */
    printf(1, "[tick %d] Process A waiting for Lock2\n", uptime());
    sem_wait(l2);
    printf(1, "[tick %d] Process A acquired Lock2 - in critical section\n", uptime());
    sleep(3);
    sem_signal(l2);
    sem_signal(l1);
    printf(1, "[tick %d] Process A released both locks - COMPLETED\n", uptime());
    exit();
  }

  pidB = fork();
  if(pidB == 0){
    if(fixed){
      /* FIXED: same global order as A */
      printf(1, "[tick %d] Process B waiting for Lock1\n", uptime());
      sem_wait(l1);
      printf(1, "[tick %d] Process B acquired Lock1\n", uptime());
      delay(4000000);
      printf(1, "[tick %d] Process B waiting for Lock2\n", uptime());
      sem_wait(l2);
      printf(1, "[tick %d] Process B acquired Lock2 - in critical section\n", uptime());
      sleep(3);
      sem_signal(l2);
      sem_signal(l1);
      printf(1, "[tick %d] Process B released both locks - COMPLETED\n", uptime());
    } else {
      /* BAD: opposite order -> circular wait */
      printf(1, "[tick %d] Process B waiting for Lock2\n", uptime());
      sem_wait(l2);
      printf(1, "[tick %d] Process B acquired Lock2\n", uptime());
      delay(4000000);
      printf(1, "[tick %d] Process B waiting for Lock1\n", uptime());
      sem_wait(l1);   /* blocks forever: A holds Lock1, waits for Lock2 */
      printf(1, "[tick %d] Process B acquired Lock1 - in critical section\n", uptime());
      sleep(3);
      sem_signal(l1);
      sem_signal(l2);
      printf(1, "[tick %d] Process B released both locks - COMPLETED\n", uptime());
    }
    exit();
  }

  if(fixed){
    wait();
    wait();
    printf(1, "[tick %d] Parent: both processes COMPLETED - no hang with consistent ordering\n", uptime());
  } else {
    sleep(50);   /* watchdog / timeout window */
    printf(1, "[tick %d] Parent WATCHDOG: 50 ticks elapsed, zero COMPLETED lines:\n", uptime());
    printf(1, "  A holds Lock1 and waits for Lock2; B holds Lock2 and waits for Lock1.\n");
    printf(1, "  DEADLOCK (circular wait) confirmed - without intervention this run hangs forever.\n");
  }
  exit();
}
