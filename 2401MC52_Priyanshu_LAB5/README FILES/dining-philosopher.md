# Assignment 5 — Question 4: Dining Philosophers on xv6-riscv

**This builds on Question 2** — it reuses `sem_init`/`sem_wait`/`sem_signal`
as-is. **No new kernel code, and no shared memory (`shm_get()`) at all,
are needed for this question.**

## Design summary

Five philosopher processes are spawned with `fork()` from one parent.
Each fork (chopstick) is represented by a **binary semaphore** — one of
the same general-purpose semaphores from Question 2, ids `0..4`,
initialized to `1` ("available"). No shared memory struct is needed:
forks live entirely as kernel-tracked semaphore state, and each
philosopher only needs to know its own id to compute which fork ids are
its "left" and "right."

Each philosopher cycles `THINKING -> HUNGRY -> EATING -> THINKING` for
`cycles` rounds (default 5, configurable: `dining 8`), printing a line
at every state transition.

## Deadlock-avoidance write-up (Resource Ordering)

**Strategy used: resource ordering — always acquire the lower-numbered
fork first.**

Philosopher `i`'s left fork is fork `i`; its right fork is fork
`(i+1) % 5`. If every philosopher picked up its **left** fork first,
all five could simultaneously grab their left fork and then block
forever waiting for their right fork — a **circular wait**, which is
exactly the classic Dining Philosophers deadlock (all 5 conditions for
deadlock — mutual exclusion, hold-and-wait, no preemption, and circular
wait — would be satisfied).

This implementation instead has every philosopher acquire
`min(left, right)` first and `max(left, right)` second — i.e. by fork
*number*, not by "left/right" role. Since fork ids form a strict total
order `0 < 1 < 2 < 3 < 4`, no philosopher can ever hold a
higher-numbered fork while waiting on a lower-numbered one that another
philosopher already holds. That makes a circular wait impossible: any
cycle in the "waiting for" graph would require some philosopher to be
waiting for a lower-numbered fork while holding a higher-numbered one,
which this ordering never allows. Breaking circular wait is sufficient
to make deadlock provably impossible, regardless of scheduling.

Concretely, this ordering only changes the behavior of **philosopher
4** (left fork = 4, right fork = 0): everyone else (0–3) still picks up
their numerically-lower left fork first as normal, but philosopher 4
picks up its *right* fork (0) before its *left* fork (4). This is the
same effect as the well-known "asymmetric" variant (one philosopher
picks up forks in the opposite order) — it falls out automatically here
from the fork-numbering rule rather than being special-cased by hand.

**On fairness:** resource ordering alone guarantees no *deadlock*; it
doesn't give a formal starvation-freedom proof the way a full
monitor-based solution (with a `state[]` array and per-philosopher
condition variables) would. In practice, with the "think"/"eat" delays
used here and only 5 philosophers, every philosopher completes all of
its cycles well within the test run — the final "completed all N
cycles" line from every philosopher, and the "no deadlock occurred"
line from the parent, are what you should point to in your log as
empirical evidence of both liveness and fairness for the assignment's
"no philosopher starves for the full test duration" requirement.

## Files

```
user/dining.c     NEW — the Dining Philosophers program
```

No kernel files are touched at all for this question.

## Integration steps

1. **Copy the new file in:**
   ```
   cp dining.c   <xv6-riscv>/user/dining.c
   ```

2. **Top-level `Makefile`** — add `dining` to `UPROGS`, the same way
   `peterson`, `prodcons`, and `readwrite` were added:
   ```make
   $U/_dining\
   ```

3. Build and run:
   ```
   make clean
   make qemu
   ```
   then at the xv6 shell:
   ```
   $ dining
   ```
   or with a custom number of cycles:
   ```
   $ dining 10
   ```

**If you hit `implicit declaration of function 'getpid'`:** same
situation as Question 3 — your xv6 variant may not expose `getpid()`.
Let me know and I'll drop the PID from the print statements (the
philosopher id alone is still enough to follow the log).

## Expected output / correctness check

You should see 5 philosophers × 5 cycles = 25 full
THINKING→HUNGRY→EATING→THINKING sequences, e.g.:

```
dining: starting 5 philosophers, 5 cycles each
Philosopher 0 (pid 4): THINKING
Philosopher 1 (pid 5): THINKING
Philosopher 2 (pid 6): THINKING
Philosopher 0 (pid 4): THINKING -> HUNGRY
Philosopher 0 (pid 4): HUNGRY -> EATING (forks 0 & 1, cycle 1/5)
Philosopher 2 (pid 6): THINKING -> HUNGRY
Philosopher 2 (pid 6): HUNGRY -> EATING (forks 2 & 3, cycle 1/5)
Philosopher 0 (pid 4): EATING -> THINKING
...
Philosopher 4 (pid 8): completed all 5 cycles
Philosopher 0 (pid 4): completed all 5 cycles
...
dining: all 5 philosophers finished 5 cycles each -- no deadlock occurred
```

For your output log, verify and note:
- **No permanent hang:** the run reaches `dining: all 5 philosophers
  finished...` — if resource ordering weren't applied (e.g. if you
  changed every philosopher to grab its *left* fork first, including
  philosopher 4), you could reproduce the classic deadlock and this
  final line would never print; the shell would just hang.
- **Two non-adjacent philosophers can eat at once:** e.g. philosophers 0
  and 2 (who share no fork — 0 uses forks 0,1; 2 uses forks 2,3) can
  have overlapping `EATING` spans in the log, since resource ordering
  only restricts adjacent philosophers, not the whole table.
- **No two adjacent philosophers eat simultaneously:** e.g. philosopher
  0 (forks 0,1) and philosopher 1 (forks 1,2) share fork 1, so
  philosopher 1's `HUNGRY -> EATING` line should never appear between
  philosopher 0's own `HUNGRY -> EATING` and `EATING -> THINKING` lines,
  and vice versa.
- **Every philosopher completes:** all 5 `Philosopher X: completed all
  N cycles` lines appear before the final summary line.

## Note on the shared semaphore table

Like the previous questions, fork ids `0..4` here come from the same
system-wide semaphore table used by `prodcons.c` (Q2, ids 0-2) and
`readwrite.c` (Q3, ids 0-2). That's fine since each program calls
`sem_init()` itself at startup — just don't run `dining` at the same
time as `prodcons` or `readwrite` in the same qemu session.
