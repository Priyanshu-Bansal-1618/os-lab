# Assignment 5 — Question 3: Readers-Writers Problem on xv6-riscv

**This builds on Questions 1 and 2** — it reuses `shm_get()` (Q1) and
`sem_init`/`sem_wait`/`sem_signal` (Q2) as-is.

## Design summary

**Shared state:** `struct rwdata { int shared_data; int read_count; }`
lives on the same kind of shared page from `shm_get()` used in Q1/Q2.
The parent creates and initializes it, then `fork()`s once per
reader/writer; each child calls `shm_get()` again to map that same
physical page (this works for any number of processes, not just one
child — `shm_get()`'s reference counting already handles that).

**Synchronization — the standard 3-semaphore solution:**
- `mutex` (id 0) — protects `read_count`, exactly as the assignment
  requires ("a `read_count` variable ... protected by a mutex").
- `wrt` (id 1) — controls exclusive access to `shared_data`. A writer
  holds it directly for its whole critical section. Readers cooperate to
  hold it *as a group*: the first reader to arrive acquires it (locking
  writers out), and the last reader to leave releases it — in between,
  any number of readers can be in their critical section at once, since
  none of them individually touch `wrt`.
- `queue` (id 2) — a fairness "turnstile" that **both readers and
  writers must pass through** before attempting `mutex`/`wrt`.


## Why the `queue` semaphore prevents writer starvation

Without it, this is the classic **first readers-writers problem**:
readers only ever contend with each other over `mutex` (held very
briefly, just to update `read_count`), so a continuous stream of
arriving readers can keep `wrt` permanently held by "the readers as a
group" — a writer waiting on `wrt` could wait forever if a new reader
always arrives before the last one leaves.

`queue` fixes this: a writer waiting on `wrt` first grabs `queue` and
holds it for its *entire* critical section (until it releases `wrt`).
Any reader that arrives while a writer holds `queue` must block on
`queue` too, *before* it ever gets to increment `read_count` — so no
new readers can sneak in ahead of a writer that's already in the queue.
Once the writer finishes and releases `queue`, waiting readers and the
next writer compete for `queue` again on equal footing. This is the
standard textbook fix for exactly the requirement in the assignment:
"readers have priority, but no reader should starve writers
indefinitely."

(Note readers still enjoy priority in the *ordinary* case — as long as
no writer is currently queued, arriving readers pass straight through
`queue` and read concurrently with no writer involved at all; `queue`
only becomes a bottleneck once a writer is actually waiting.)

## Demonstrating the required behaviors

- **Concurrent readers:** each reader prints `ENTER round N (active
  readers = k)` right after incrementing `read_count` inside `mutex`,
  and `EXIT round N (active readers = k)` right before decrementing. If
  you see `active readers = 2` or `3` in these lines, or interleaved
  `ENTER`s from different reader PIDs before the matching `EXIT`s
  appear, that's direct evidence of concurrent reading.
- **Exclusive writers:** each writer prints a tightly-bracketed `ENTER
  (exclusive)` ... `wrote shared_data = X` ... `EXIT (exclusive)`
  sequence with a `delay()` in between. Because `wrt` is held for that
  whole span, no reader's `ENTER`/read/`EXIT` lines and no other
  writer's `ENTER`/`EXIT` lines can appear between a writer's own
  `ENTER` and `EXIT` in the log.
- Every line includes both a **PID** (`getpid()`) and a **reader/writer
  id + round number**, satisfying "output log with timestamps/PIDs
  showing correct interleaving" (this environment has no reliable
  ticks-based clock available to user programs — see the note on
  `sleep()` from Question 2 — so PID + round number is used instead of a
  wall-clock timestamp to identify and order events).

## Note on the shared semaphore table

Like `shm_get()` from Question 1, the semaphore ids (0/1/2) here are
from the same system-wide table used by `prodcons.c` in Question 2.
