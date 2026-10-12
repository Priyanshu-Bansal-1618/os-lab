# Consolidated Report - Assignment 7
**xv6: Threads, Synchronization, Deadlock, and Scheduling**
* **Student:** Priyanshu Bansal | 2401MC52 | MA3104 | Operating System
* **Environment:** xv6 (public) on QEMU

---

## 1. Kernel-Supported Threads (`clone` and `join`)

### System Design
To implement threading in xv6, I modified the system so that threads act as lightweight processes sharing the same memory space as their parent. 

* **The Thread Structure:** A thread is essentially a `struct proc` with a new flag `isthread = 1`. Unlike a standard process, a thread **shares** the Page Directory (`pgdir`) of its creator instead of copying it (skipping `copyuvm`). However, `allocproc()` still assigns each thread its own private kernel stack to prevent execution overlap.
* **The `clone` System Call (Syscall 22):** The `clone(fcn, arg, stack)` function sets up the trapframe so the thread knows exactly where to start executing. It sets the instruction pointer (`eip`) to the target function, sets the stack pointer (`esp`) to the top of the user-provided stack page, and pushes the argument and a fake return address following the standard cdecl calling convention.
* **File Sharing:** To ensure functions like `printf` work correctly inside threads, the open-file table is duplicated using `filedup`.
* **Cleanup (`join` and `wait`):** 
  * The `join()` syscall (Syscall 23) is designed to reap only zombie processes that are flagged as threads. It frees the thread's kernel stack but strictly avoids freeing the shared page directory.
  * The standard `wait()` syscall was modified to ignore threads entirely, ensuring it never accidentally destroys the shared memory space while other threads are still using it.
* **Forking from a Thread:** If a thread calls `fork()`, it generates a standard, isolated process with its own memory copy (`isthread = 0`).

### Test Results
To verify the implementation, I ran a test where three separate threads incremented a single shared counter. Because all threads share the same address space, the parent process could read the final counter value. **Without synchronization**, the final value was unpredictable due to race conditions (lost updates when multiple threads try to write simultaneously). A correct, maximum value of 300,000 could only be reached coincidentally if the system serialized the runs, proving that the threads were actively sharing memory.

---

## 2. Counting Semaphores & Synchronization

### System Design
To fix the race conditions observed in the thread tests, I implemented counting semaphores to enforce mutual exclusion.

* **Data Structure:** I created a `struct semaphore` containing an integer `value`, a `spinlock` for safe access, and an `inuse` flag. The system maintains a fixed global array of these semaphores (`sems[10]`).
* **System Calls:**
  * `sem_init` (Syscall 24): Initializes a semaphore with a starting value.
  * `sem_wait` (Syscall 25): Blocks the thread using `sleep()` if the semaphore value is `0` or less. Once the thread wakes up and the value is greater than zero, it decrements the value.
  * `sem_signal` (Syscall 26): Increments the semaphore value and calls `wakeup()`. 
* **Safe Wakeups:** Because xv6's `wakeup()` function wakes *all* sleeping processes, I used a `while (value <= 0)` loop in `sem_wait`. This ensures that when multiple threads wake up, they re-check the condition, and only one thread successfully proceeds, avoiding race conditions.

### Results & Producer-Consumer Problem
I ran the exact same 3-thread, 100,000-increment workload from Q1, this time protected by the new semaphores.

| Workload Condition | Final Counter Value | Analysis |
| :--- | :--- | :--- |
| **Before (No Locks)** | Unreliable (Below 300,000) | Failed due to race conditions and lost updates. |
| **After (Semaphores)** | Exactly 300,000 | Mutual exclusion successfully protected the shared data. |

**Producer-Consumer Implementation:**
I used the semaphores to solve a bounded buffer problem (buffer size of 5) managing 20 total items. I configured three semaphores: `EMPTY = 5`, `FULL = 0`, and `MUTEX = 1`. 
* The producer blocks automatically if there are 5 unconsumed items, preventing buffer overflow.
* The consumer blocks if the buffer is empty, preventing underflow. 
* The system successfully processed items in strict FIFO order (1 through 20), printing each slot interaction cleanly to the log.

---

## 3. Deadlock Detection and Avoidance

### System Design
I implemented deadlock management mechanisms, specifically focusing on work-vector scanning and Banker's Algorithm.

* **Detection Algorithm:** The `detect_deadlock()` function scans the system's state. It looks for any process whose current requests can be satisfied by the available resources (`Request <= work`). If a process can finish, its allocated resources are added back to the "available" pool. Any processes left unfinished at the end of the scan are mathematically deadlocked.

### Scenario Testing

| Scenario | System State Setup | Result |
| :--- | :--- | :--- |
| **T1: Safe State** | Avail(1,1); Alloc {0,1},{1,0},{0,0}; Req {1,0},{0,1},{1,1} | **NO DEADLOCK:** All processes can satisfy requirements and finish. |
| **T2: Deadlock** | Avail(0,0); Alloc {1,0},{0,1}; Req {0,1},{1,0} | **DEADLOCK:** Neither process can proceed. Detected {P0, P1}. |
| **T3: Banker's Avoidance** | Avail(1,1); Alloc {1,1},{1,1}; Max {3,3},{2,2} | **AVOIDED:** P1's request is granted. P0's request is denied because granting it would push the system into an unsafe state. |

**Real-World Pipe Deadlock Test:**
To test this in a real scenario, I created two processes (A and B). Process A held the write-end of Pipe 1 while waiting to read Pipe 2. Process B held the write-end of Pipe 2 while waiting to read Pipe 1. `detect_deadlock()` successfully identified the circular wait (A -> B -> A). I then used `kill()` followed by `wait()` to break the deadlock, utilizing xv6's built-in mechanic where `piperead` returns `-1` if the reading process is killed.

---

## 4. Priority Preemptive Scheduling with Aging

### System Design
The default xv6 scheduler is a simple Round-Robin. I replaced it with a preemptive priority scheduler that includes an anti-starvation mechanism (aging).

* **Priority Levels:** Added a `priority` field to `struct proc` ranging from 1 (highest priority) to 20 (lowest priority), defaulting to 10. Added a `waitticks` counter to track how long a process waits in the queue.
* **The Scheduler:** The `scheduler()` loops through the process table and selects the `RUNNABLE` process with the **lowest priority number** (highest actual priority). It uses a rotating cursor (`rr_idx`) so that processes with the exact same priority are executed in a Round-Robin fashion. Once a process is dispatched, its `waitticks` reset to zero.
* **Aging (Starvation Prevention):** To ensure low-priority tasks aren't ignored forever by high-priority tasks, `age_procs()` runs on every hardware timer tick. It increments `waitticks` for all waiting `RUNNABLE` processes. If a process waits for more than 30 ticks, its priority number is decremented (effectively boosting its priority) and its timer resets.

### Observed Results (2-CPU QEMU)
I tested the scheduler using processes initialized with different priorities. 

| Process | Assigned Priority | Ticks to Finish | System Behavior |
| :--- | :--- | :--- | :--- |
| **proc0** | 1 (Highest) | ~100-110 ticks | Picked up and executed immediately. |
| **proc1** | 5 | ~100-110 ticks | Picked up and executed immediately. |
| **proc2** | 10 (Default) | ~200 ticks | Held back until the higher-priority processes finished. |
| **proc3** | 20 (Lowest) | ~300 ticks | Priority actively stepped up (20 -> 19 -> 18) via the aging mechanism until it was allowed to run. |

The completion order strictly correlated with priority rankings. Most importantly, the lowest-priority process successfully finished, proving that the 30-tick aging rule prevents CPU starvation.

---

## 5. Challenges Faced and Engineering Resolutions

1. **Silent Threads:** Initially, the newly created threads were not printing anything to the console. I realized `clone()` was not duplicating the open-file table, meaning file descriptor 1 (standard output) was closed inside the threads. I resolved this by copying the `filedup()` loop mechanism found in `fork()` into `clone()`.
2. **The `wakeup()` Broadcast Issue:** xv6's `wakeup()` function wakes *all* sleeping processes at once. Initially, my semaphores simply decremented and slept if the value was below zero. When multiple threads woke up simultaneously, they all proceeded, causing race conditions. I fixed this by implementing a `while (value <= 0)` sleep loop so threads re-evaluate the semaphore value immediately upon waking.
3. **Masked Race Conditions:** Testing on a single-CPU QEMU instance accidentally serialized the execution, making the threaded counter artificially reach 300,000 even without locks. I had to force wider race windows and switch to SMP (Symmetric Multiprocessing) runs to expose the lost updates, which allowed me to properly verify my semaphore logic.
4. **Unwinding Pipe Deadlocks:** A genuine pipe deadlock cannot resolve itself. I utilized `kill()` to terminate the deadlocked processes. Because xv6's `piperead` gracefully returns `-1` for killed readers, the child processes were able to exit the deadlock cleanly without crashing the OS.
5. **Strict Priority Starvation:** Early testing of the priority scheduler showed that tasks with a priority of 20 would never run if tasks with a priority of 1 were constantly active. Implementing the 30-tick aging rule dynamically boosted low-priority tasks, elegantly solving the starvation issue.