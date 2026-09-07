# Assignment 5 — Question 2: Producer-Consumer (Bounded Buffer) on xv6-riscv

**This builds on Question 1** — it reuses the `shm_get()` syscall from
Question 1 for the shared buffer.

## Design summary

**Shared buffer:** the producer (parent) and consumer (child) share a
circular buffer via the same `shm_get()` mechanism from Question 1 — the
parent calls `shm_get()` to create+map the shared page, lays out a
`struct pcbuf` (bufsize, in/out indices, and the buffer array) on it,
then `fork()`s; the child calls `shm_get()` again to map that same
physical page. Buffer size is configurable via a command-line argument
(`prodcons 8`), default 5, capped at 100 to fit comfortably in one 4KB
page.

**Synchronization:** rather than busy-waiting, this implements a real
*blocking* counting semaphore as three new syscalls
(`sem_init`/`sem_wait`/`sem_signal`, in the new `kernel/sem.c`), built
directly on xv6's own `sleep()`/`wakeup()` — the same primitive xv6
itself uses internally for pipes and `wait()`. A waiting process is
actually taken off the run queue, not spinning, so `top`/scheduler
behavior looks the same as any other blocked xv6 process.

Three semaphores are used, exactly as the assignment specifies two
counting semaphores + one mutex:
- `empty` (id 0) — counts free slots, initialized to `bufsize`.
- `full` (id 1) — counts filled slots, initialized to `0`.
- `mutex` (id 2) — binary semaphore (initialized to `1`) protecting the
  buffer during insert/remove, implemented with the same semaphore
  primitive rather than a separate lock type, as the assignment permits
  ("one mutex/lock").

Classic bounded-buffer protocol:
```
producer: wait(empty); wait(mutex); insert;  signal(mutex); signal(full);
consumer: wait(full);  wait(mutex); remove;  signal(mutex); signal(empty);
```

**Demonstrating blocking:** the producer sleeps 1 tick and the consumer
sleeps 3 ticks between operations, so:
- At start-up `full = 0`, so the **consumer's first `wait(full)` blocks**
  until the producer inserts the first item — this shows the
  buffer-empty blocking case immediately in the log.
- Because the consumer is slower, the buffer **fills up** partway
  through the run, and the **producer's `wait(empty)` blocks** until the
  consumer frees a slot — this shows the buffer-full blocking case.

Each print statement also shows an *informal* buffer-occupancy count
(`approx_count()`) purely for log readability — it is not used for any
correctness decision; all real synchronization is via the semaphores.

## Why a real (blocking) semaphore instead of busy-waiting

The assignment asks to *demonstrate* the producer blocking when
full and the consumer blocking when empty. A busy-wait (spin loop, as
used for Peterson's Algorithm in Question 1) would technically enforce
the same ordering, but the waiting process would burn CPU the entire
time instead of actually blocking — which isn't a faithful
implementation of "the producer blocks" / "the consumer blocks" as
those terms are normally meant in OS courses. `sem_wait()` here calls
the kernel's own `sleep(chan, lock)`, which takes the process off the
run queue entirely (state `SLEEPING`) until `sem_signal()`'s `wakeup()`
puts it back on the run queue — this is the same mechanism xv6 uses
internally for pipe reads/writes and `wait()`.
