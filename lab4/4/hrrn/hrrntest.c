#include "types.h"
#include "stat.h"
#include "user.h"

void
cpu_bound_work(int units)
{
  volatile double x = 0;
  int i;

  /* Keep the workload CPU-bound while making the relative burst sizes
   * easy to control for the scheduler test. */
  for(i = 0; i < units * 1000000; i++)
    x += 3.14 * 2.71;
}

void
delay_loop(int units)
{
  volatile int x = 0;
  int i;

  for(i = 0; i < units * 1000000; i++)
    x += i;
}

int
main(void)
{
  int pid;
  int i;

  printf(1, "Starting HRRN Scheduler Test...\n");

  /* Long job arrives first. */
  pid = fork();
  if(pid == 0){
    int mypid = getpid();
    set_burst(mypid, 40);
    printf(1, "Long job (PID: %d, BT: 40) started\n", mypid);
    cpu_bound_work(40);
    printf(1, "Long job (PID: %d) finished\n", mypid);
    exit();
  }
  if(pid < 0){
    printf(1, "Fork failed!\n");
    exit();
  }

  /* Stagger the next arrivals. */
  delay_loop(2);

  /* Short job arrives later and should normally be favored initially. */
  pid = fork();
  if(pid == 0){
    int mypid = getpid();
    set_burst(mypid, 5);
    printf(1, "Short job (PID: %d, BT: 5) started\n", mypid);
    cpu_bound_work(5);
    printf(1, "Short job (PID: %d) finished\n", mypid);
    exit();
  }
  if(pid < 0){
    printf(1, "Fork failed!\n");
    exit();
  }

  delay_loop(2);

  /* Medium job arrives last. */
  pid = fork();
  if(pid == 0){
    int mypid = getpid();
    set_burst(mypid, 15);
    printf(1, "Medium job (PID: %d, BT: 15) started\n", mypid);
    cpu_bound_work(15);
    printf(1, "Medium job (PID: %d) finished\n", mypid);
    exit();
  }
  if(pid < 0){
    printf(1, "Fork failed!\n");
    exit();
  }

  for(i = 0; i < 3; i++)
    wait();

  printf(1, "HRRN Scheduler Test completed.\n");
  exit();
}
