// user/prodcons.c  --  NEW FILE
//
// Question 2: Producer-Consumer with a bounded circular buffer, built on
// top of:
//   - shm_get()               (Question 1's syscall) for the shared
//                              buffer between the two processes, and
//   - sem_init/sem_wait/sem_signal   (new, kernel/sem.c) for real
//                              blocking synchronization.
//
// Usage:  prodcons [bufsize]        (default bufsize = 5)
//
// Producer (parent) inserts integers 1..NUM_ITEMS into the buffer.
// Consumer (child) removes them and prints them. Two counting
// semaphores -- "empty" (slots available) and "full" (items available)
// -- plus a binary semaphore used as a mutex protect the buffer, exactly
// as in the classic bounded-buffer solution:
//
//   producer: wait(empty); wait(mutex); insert; signal(mutex); signal(full);
//   consumer: wait(full);  wait(mutex); remove; signal(mutex); signal(empty);
//
// The two sides are deliberately given different delays between
// operations (busy-wait loops -- see the delay() function below, since
// this kernel has no ticks-based sleep(n) syscall) so you can SEE both
// blocking cases in the output:
//   - At start-up, full=0, so the consumer's first wait(full) blocks
//     until the producer inserts the very first item (buffer-empty
//     blocking).
//   - The consumer is made slower than the producer, so the buffer
//     fills up and the producer's wait(empty) blocks until the consumer
//     frees a slot (buffer-full blocking).
 
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
 
#define NUM_ITEMS        20
#define DEFAULT_BUFSIZE   5
#define MAX_BUFSIZE     100
 
#define SEM_EMPTY 0   // counts free slots
#define SEM_FULL  1   // counts filled slots
#define SEM_MUTEX 2   // binary semaphore guarding the buffer
 
// This kernel has no ticks-based sleep(n) syscall for user programs, so
// "sleeping briefly" between insertions/removals is done with a plain
// busy-wait delay loop instead -- the same technique already used for
// the remainder-section delay in peterson.c. It doesn't yield the CPU,
// but xv6's timer-interrupt-driven preemptive scheduler still lets the
// producer and consumer take turns, which is all that's needed here to
// make the two sides run at different relative speeds.
static void
delay(int n)
{
  for(volatile int i = 0; i < n; i++)
    ;
}
 
struct pcbuf {
  int bufsize;          // actual buffer size in use (<= MAX_BUFSIZE)
  int in;                // next slot to write
  int out;                // next slot to read
  int buf[MAX_BUFSIZE];
};
 
// Informal, lock-free peek at how full the buffer looks, for print
// messages only -- NOT used for any correctness decision (those are
// all made by the semaphores above).
static int
approx_count(struct pcbuf *b)
{
  int c = b->in - b->out;
  if(c < 0)
    c += b->bufsize;
  return c;
}
 
int
main(int argc, char *argv[])
{
  int bufsize = DEFAULT_BUFSIZE;
  if(argc > 1){
    bufsize = atoi(argv[1]);
    if(bufsize < 1 || bufsize > MAX_BUFSIZE){
      printf("prodcons: bufsize must be between 1 and %d\n", MAX_BUFSIZE);
      exit(1);
    }
  }
 
  uint64 va = shm_get();
  if(va == 0xffffffffffffffff){
    printf("prodcons: shm_get failed\n");
    exit(1);
  }
  struct pcbuf *b = (struct pcbuf *)va;
  b->bufsize = bufsize;
  b->in = 0;
  b->out = 0;
 
  // Parent (about to fork) sets up the semaphores once, before either
  // side touches them.
  sem_init(SEM_EMPTY, bufsize);  // 'bufsize' free slots at the start
  sem_init(SEM_FULL, 0);          // 0 items available at the start
  sem_init(SEM_MUTEX, 1);         // mutex starts "unlocked"
 
  printf("prodcons: starting with bufsize=%d, %d items\n", bufsize, NUM_ITEMS);
 
  int pid = fork();
  if(pid < 0){
    printf("prodcons: fork failed\n");
    exit(1);
  } else if(pid == 0){
    // ---------------- CONSUMER (child) ----------------
    va = shm_get();               // map the SAME shared page
    if(va == 0xffffffffffffffff){
      printf("prodcons: consumer shm_get failed\n");
      exit(1);
    }
    b = (struct pcbuf *)va;
 
    for(int i = 0; i < NUM_ITEMS; i++){
      sem_wait(SEM_FULL);          // blocks here if buffer is empty
      sem_wait(SEM_MUTEX);
 
      int item = b->buf[b->out];
      b->out = (b->out + 1) % b->bufsize;
      printf("Consumer: removed %d (buffer now ~%d/%d)\n",
             item, approx_count(b), b->bufsize);
 
      sem_signal(SEM_MUTEX);
      sem_signal(SEM_EMPTY);
 
      delay(600000);                // consumer is the slower side
    }
    printf("Consumer: done, consumed %d items\n", NUM_ITEMS);
    exit(0);
 
  } else {
    // ---------------- PRODUCER (parent) ----------------
    for(int i = 1; i <= NUM_ITEMS; i++){
      sem_wait(SEM_EMPTY);          // blocks here if buffer is full
      sem_wait(SEM_MUTEX);
 
      b->buf[b->in] = i;
      b->in = (b->in + 1) % b->bufsize;
      printf("Producer: inserted %d (buffer now ~%d/%d)\n",
             i, approx_count(b), b->bufsize);
 
      sem_signal(SEM_MUTEX);
      sem_signal(SEM_FULL);
 
      delay(200000);                // producer is the faster side
    }
    printf("Producer: done, produced %d items\n", NUM_ITEMS);
 
    wait(0);                        // wait for consumer to finish too
    printf("prodcons: both processes finished\n");
  }
 
  exit(0);
}
 
