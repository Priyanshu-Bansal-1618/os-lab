#include "types.h"
#include "stat.h"
#include "user.h"

/* CPU-bound work intentionally does not yield or sleep.  The timer interrupt
 * therefore forces it to consume complete quanta and move down the MLFQ. */
void
cpu_bound_work(void)
{
  volatile int x = 0;
  for(;;)
    x++;
}

/* I/O-bound work frequently sleeps.  It should give up the CPU voluntarily
 * before consuming a complete quantum and therefore remain in the high queue. */
void
io_bound_work(void)
{
  int i;

  for(i = 0; i < 12; i++){
    printf(1, "I/O-bound process (PID: %d) iteration %d\n",
           getpid(), i + 1);
    sleep(2);
  }

  printf(1, "I/O-bound process (PID: %d) finished\n", getpid());
  exit();
}

void
delay_loop(void)
{
  volatile int x = 0;
  int i;

  for(i = 0; i < 3000000; i++)
    x += i;
}

int
main(void)
{
  int cpu1, io, cpu2;

  printf(1, "Starting MLFQ Scheduler Test...\n");
  printf(1, "Queue levels: 0 (1 tick), 1 (2 ticks), 2 (4 ticks)\n");

  /* First CPU-bound process starts in queue 0 and should be demoted as it
   * repeatedly consumes its full quantum. */
  cpu1 = fork();
  if(cpu1 == 0){
    printf(1, "CPU-bound process 1 (PID: %d) started\n", getpid());
    cpu_bound_work();
  }
  if(cpu1 < 0){
    printf(1, "Fork failed for CPU-bound process 1!\n");
    exit();
  }

  delay_loop();

  /* Interactive/I/O-bound process arrives later and repeatedly sleeps. */
  io = fork();
  if(io == 0)
    io_bound_work();
  if(io < 0){
    printf(1, "Fork failed for I/O-bound process!\n");
    kill(cpu1);
    wait();
    exit();
  }

  delay_loop();

  /* Second CPU-bound process demonstrates round-robin behavior within the
   * same queue before it too sinks through the feedback levels. */
  cpu2 = fork();
  if(cpu2 == 0){
    printf(1, "CPU-bound process 2 (PID: %d) started\n", getpid());
    cpu_bound_work();
  }
  if(cpu2 < 0){
    printf(1, "Fork failed for CPU-bound process 2!\n");
    kill(cpu1);
    wait();
    wait();
    exit();
  }

  /* Wait for the finite I/O-bound workload to finish.  During this period
   * the kernel prints each CPU-bound demotion (queue 0 -> 1 -> 2). */
  wait();

  printf(1, "I/O-bound workload completed; stopping CPU-bound test jobs.\n");
  kill(cpu1);
  kill(cpu2);
  wait();
  wait();

  printf(1, "MLFQ Scheduler Test completed.\n");
  exit();
}
