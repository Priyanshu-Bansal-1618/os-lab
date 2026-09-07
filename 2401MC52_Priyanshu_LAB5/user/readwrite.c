// user/readwrite.c  --  NEW FILE
//
// Question 3: Readers-Writers Problem.
//
// Implements the FIRST readers-writers problem (readers may run
// concurrently and are not blocked by other readers), with the classic
// three-semaphore "queue"/turnstile addition so a writer that is
// already waiting cannot be starved by a never-ending stream of new
// readers -- see the README for why this specific addition fixes
// starvation while still giving readers priority in the common case.
//
// Reuses:
//   - shm_get()                         from Question 1, for the page
//     shared by every reader/writer process.
//   - sem_init/sem_wait/sem_signal      from Question 2 (kernel/sem.c),
//     for synchronization. No new kernel code is needed for this
//     question at all.
//
// Spawns NUM_READERS readers and NUM_WRITERS writers with fork(), each
// running ROUNDS rounds of read/write.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define NUM_READERS 3
#define NUM_WRITERS 2
#define ROUNDS      5

#define SEM_MUTEX 0   // protects read_count
#define SEM_WRT   1   // exclusive access to shared_data: held by writers,
                       // and by the readers as a group (first reader in
                       // acquires it, last reader out releases it)
#define SEM_QUEUE 2   // fairness turnstile -- see README

struct rwdata {
  int shared_data;
  int read_count;      // number of readers currently in the CS; guarded by SEM_MUTEX
};

static void
delay(int n)
{
  for(volatile int i = 0; i < n; i++)
    ;
}

static void
reader(struct rwdata *d, int id)
{
  int pid = getpid();

  for(int r = 0; r < ROUNDS; r++){
    // ---- entry section ----
    sem_wait(SEM_QUEUE);            // fairness gate (see README)
      sem_wait(SEM_MUTEX);
        d->read_count++;
        if(d->read_count == 1)
          sem_wait(SEM_WRT);        // first reader locks writers out
        printf("Reader %d (pid %d): ENTER  round %d (active readers = %d)\n",
               id, pid, r, d->read_count);
      sem_signal(SEM_MUTEX);
    sem_signal(SEM_QUEUE);

    // ---- critical section (read-only, so readers may overlap) ----
    delay(300000);                   // simulate time spent reading
    printf("Reader %d (pid %d): read shared_data = %d\n", id, pid, d->shared_data);

    // ---- exit section ----
    sem_wait(SEM_MUTEX);
      d->read_count--;
      printf("Reader %d (pid %d): EXIT   round %d (active readers = %d)\n",
             id, pid, r, d->read_count);
      if(d->read_count == 0)
        sem_signal(SEM_WRT);         // last reader lets writers back in
    sem_signal(SEM_MUTEX);

    // ---- remainder / "think" time before the next round ----
    delay(150000 + 20000 * id);
  }
}

static void
writer(struct rwdata *d, int id)
{
  int pid = getpid();

  for(int r = 0; r < ROUNDS; r++){
    // ---- entry section ----
    sem_wait(SEM_QUEUE);             // fairness gate (see README)
      sem_wait(SEM_WRT);              // exclusive access: no reader, no other writer

      // ---- critical section ----
      printf("Writer %d (pid %d): ENTER  round %d (exclusive)\n", id, pid, r);
      delay(400000);                  // simulate time spent writing
      d->shared_data = d->shared_data + 1;
      printf("Writer %d (pid %d): wrote shared_data = %d\n", id, pid, d->shared_data);
      printf("Writer %d (pid %d): EXIT   round %d (exclusive)\n", id, pid, r);

      // ---- exit section ----
      sem_signal(SEM_WRT);
    sem_signal(SEM_QUEUE);

    // ---- remainder / "think" time before the next round ----
    delay(200000 + 20000 * id);
  }
}

int
main(int argc, char *argv[])
{
  uint64 va = shm_get();
  if(va == 0xffffffffffffffff){
    printf("readwrite: shm_get failed\n");
    exit(1);
  }
  struct rwdata *d = (struct rwdata *)va;
  d->shared_data = 0;
  d->read_count = 0;

  // Parent sets up all synchronization primitives once, before forking
  // any reader/writer child.
  sem_init(SEM_MUTEX, 1);
  sem_init(SEM_WRT, 1);
  sem_init(SEM_QUEUE, 1);

  printf("readwrite: starting %d readers and %d writers, %d rounds each\n",
         NUM_READERS, NUM_WRITERS, ROUNDS);

  int total = NUM_READERS + NUM_WRITERS;
  for(int i = 0; i < total; i++){
    int pid = fork();
    if(pid < 0){
      printf("readwrite: fork failed\n");
      exit(1);
    } else if(pid == 0){
      // Child: map the SAME shared page the parent created.
      va = shm_get();
      if(va == 0xffffffffffffffff){
        printf("readwrite: child shm_get failed\n");
        exit(1);
      }
      d = (struct rwdata *)va;

      if(i < NUM_READERS)
        reader(d, i);
      else
        writer(d, i - NUM_READERS);

      exit(0);
    }
    // parent: loop around to fork the next child
  }

  for(int i = 0; i < total; i++)
    wait(0);

  printf("readwrite: all done. final shared_data = %d (expected %d)\n",
         d->shared_data, NUM_WRITERS * ROUNDS);
  exit(0);
}