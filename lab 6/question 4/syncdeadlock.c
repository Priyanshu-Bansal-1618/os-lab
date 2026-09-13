#include "types.h"
#include "stat.h"
#include "user.h"

#define NP 5
#define NRES 3
#define CYCLES 4

#define MUTEX 3          /* protects the avail mirror */

static const char *rname[NRES] = { "printer", "scanner", "disk" };

/* each process needs 2 of the 3 types; pairs stored in ASCENDING order
   (printer=0 < scanner=1 < disk=2) = the resource-ordering strategy */
static int want[NP][2] = {
  {0,1}, {1,2}, {0,2}, {0,1}, {1,2}
};

struct shminfo { int avail[NRES]; };

int
main(void)
{
  struct shminfo *shm;
  int pid, i, k, id;

  /* resource pools = counting semaphores (instance limits 2,1,2) */
  sem_init(0, 2);        /* printer */
  sem_init(1, 1);        /* scanner */
  sem_init(2, 2);        /* disk    */
  sem_init(MUTEX, 1);

  shm = (struct shminfo*)shm_get();
  shm->avail[0] = 2; shm->avail[1] = 1; shm->avail[2] = 2;

  id = -1;
  for(i = 0; i < NP; i++){
    pid = fork();
    if(pid == 0){ id = i; break; }
  }

  if(id >= 0){
    shm = (struct shminfo*)shm_get();   /* child remaps to shared page */
    for(k = 1; k <= CYCLES; k++){
      int a = want[id][0], b = want[id][1];   /* a < b : global order */

      sleep(2 + id);                     /* stagger arrivals */

      printf(1, "[tick %d] P%d requests %s\n", uptime(), id, rname[a]);
      sem_wait(a);
      sem_wait(MUTEX);
      shm->avail[a]--;
      printf(1, "[tick %d] P%d granted %s (%s avail now %d)\n",
             uptime(), id, rname[a], rname[a], shm->avail[a]);
      sem_signal(MUTEX);

      printf(1, "[tick %d] P%d requests %s\n", uptime(), id, rname[b]);
      sem_wait(b);
      sem_wait(MUTEX);
      shm->avail[b]--;
      printf(1, "[tick %d] P%d granted %s (%s avail now %d)\n",
             uptime(), id, rname[b], rname[b], shm->avail[b]);
      sem_signal(MUTEX);

      printf(1, "[tick %d] P%d WORKING with %s+%s\n",
             uptime(), id, rname[a], rname[b]);
      sleep(8);                          /* simulated work */

      sem_wait(MUTEX);
      shm->avail[a]++; shm->avail[b]++;
      printf(1, "[tick %d] P%d releasing %s+%s (avail now %d,%d,%d)\n",
             uptime(), id, rname[a], rname[b],
             shm->avail[0], shm->avail[1], shm->avail[2]);
      sem_signal(MUTEX);
      sem_signal(a);
      sem_signal(b);
    }
    printf(1, "P%d COMPLETED %d cycles\n", id, CYCLES);
    exit();
  }

  for(i = 0; i < NP; i++)
    wait();
  printf(1, "All 5 processes completed %d cycles - no deadlock, instances never exceeded.\n", CYCLES);
  exit();
}
