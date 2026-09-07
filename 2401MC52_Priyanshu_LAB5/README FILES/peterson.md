# Assignment 5 — Question 1: Peterson's Algorithm on xv6-riscv

## Design summary

xv6 processes do not share memory after `fork()` — the child gets a full
private copy of the parent's address space (`uvmcopy`). To run Peterson's
Algorithm we first need a page both processes can actually see and write
to, so this solution adds **one new syscall**, `shm_get()`:

- The first process to call it (the parent, in `peterson.c`, before
  `fork()`) has the kernel `kalloc()` a fresh physical page and map it
  into its page table at a fixed virtual address, `SHMVA`
  (defined in `kernel/memlayout.h` as `TRAPFRAME - 2*PGSIZE`, an address
  that's unused by normal user programs and safely below the trapframe/
  trampoline mappings).
- After `fork()`, the child calls `shm_get()` again. The kernel sees the
  page already exists (tracked in a small global in `kernel/shm.c`) and
  just maps that *same physical page* into the child's own page table at
  the same `SHMVA`. Both processes now read/write the same RAM.
- The page is reference-counted and freed automatically once both
  processes have exited (`shm_unmap()`, hooked into `freeproc()`).

On top of that page, `user/peterson.c` lays out:

```c
struct shared {
  int flag[2];
  int turn;
  int shared_counter;
};
```

and implements the textbook Peterson's Algorithm entry/exit sections
using **only** `flag[]`/`turn` — no xv6 lock is used to enforce the
mutual exclusion itself, per the assignment's requirement. Process 0
(parent) and process 1 (child) each run 10 iterations, incrementing
`shared_counter` inside the critical section and printing
`Process <id> in CS, counter = <value>`.

`__sync_synchronize()` compiler/CPU memory barriers are placed around the
shared-variable accesses. This matters because xv6-riscv can run with
multiple harts (CPUs), so the parent and child may genuinely execute in
parallel on different cores, not just be interleaved by a single
scheduler — without barriers, Peterson's Algorithm is not guaranteed
correct on a real multi-core memory model.

## Why no OS lock was used for the mutual exclusion itself

The critical section entry/exit is enforced purely by `flag[0]`,
`flag[1]`, and `turn` in the shared page, as Peterson's Algorithm
requires. The only kernel involvement is `shm_get()`, which merely
*provides the shared memory* (the substrate the algorithm runs on) —
it grants no synchronization guarantee by itself.
