#include "types.h"
#include "stat.h"
#include "user.h"

void
cpu_bound_work(int loops)
{
  volatile double x = 0;
  int i;
  for(i = 0; i < loops; i++)
    x += 3.14 * 2.71;
}

void
delay_loop(int loops)
{
  volatile int x = 0;
  int i;
  for(i = 0; i < loops; i++)
    x++;
}

int
main(void)
{
  int pid;

  printf(1, "Starting SRTF Scheduler Test...\n");

  // Long job arrives first.
  pid = fork();
  if(pid == 0){
    if(set_burst(getpid(), 40) < 0){
      printf(1, "set_burst failed for long job\n");
      exit();
    }
    printf(1, "Long job  (PID: %d, BT: 40) started\n", getpid());
    cpu_bound_work(90000000);
    printf(1, "Long job  (PID: %d) finished\n", getpid());
    exit();
  }
  if(pid < 0){
    printf(1, "Fork failed!\n");
    exit();
  }

  // Stagger the arrivals so the long job is already executing.
  delay_loop(20000000);

  // Short job arrives later and should preempt the long job.
  pid = fork();
  if(pid == 0){
    if(set_burst(getpid(), 8) < 0){
      printf(1, "set_burst failed for short job\n");
      exit();
    }
    printf(1, "Short job (PID: %d, BT: 8) arrived\n", getpid());
    cpu_bound_work(20000000);
    printf(1, "Short job (PID: %d) finished\n", getpid());
    exit();
  }
  if(pid < 0){
    printf(1, "Fork failed!\n");
    exit();
  }

  delay_loop(20000000);

  // Medium job arrives after the short job.
  pid = fork();
  if(pid == 0){
    if(set_burst(getpid(), 16) < 0){
      printf(1, "set_burst failed for medium job\n");
      exit();
    }
    printf(1, "Medium job(PID: %d, BT: 16) arrived\n", getpid());
    cpu_bound_work(40000000);
    printf(1, "Medium job(PID: %d) finished\n", getpid());
    exit();
  }
  if(pid < 0){
    printf(1, "Fork failed!\n");
    exit();
  }

  wait();
  wait();
  wait();

  printf(1, "SRTF Scheduler Test completed.\n");
  exit();
}
