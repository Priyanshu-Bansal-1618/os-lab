#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void lock_init(int p[2]) {
    pipe(p);
    write(p[1], "x", 1);
}

void lock_acquire(int p[2]) {
    char c;
    read(p[0], &c, 1);
}

void lock_release(int p[2]) {
    write(p[1], "x", 1);
}

int main() {
    int lock1[2], lock2[2];
    lock_init(lock1);
    lock_init(lock2);

    printf("\n=== Running FIXED Scenario (Resource Ordering) ===\n");

    int pidA = fork();
    if (pidA == 0) {
        // PROCESS A (Acquires Lock 1, then Lock 2)
        lock_acquire(lock1);
        printf("[Process A] Acquired Lock 1\n");
        
        sleep(10); 
        
        printf("[Process A] Waiting for Lock 2...\n");
        lock_acquire(lock2);
        printf("[Process A] Acquired Lock 2!\n");
        
        // Do work, then release
        lock_release(lock2);
        lock_release(lock1);
        printf("[Process A] Finished and released locks.\n");
        exit(0);
    }

    int pidB = fork();
    if (pidB == 0) {
        // PROCESS B (FIXED: Now acquires Lock 1 first, just like Process A)
        lock_acquire(lock1);
        printf("[Process B] Acquired Lock 1\n");
        
        sleep(10); 
        
        printf("[Process B] Waiting for Lock 2...\n");
        lock_acquire(lock2);
        printf("[Process B] Acquired Lock 2!\n");
        
        // Do work, then release
        lock_release(lock2);
        lock_release(lock1);
        printf("[Process B] Finished and released locks.\n");
        exit(0);
    }

    wait(0);
    wait(0);
    
    printf("SUCCESS: All processes finished cleanly without deadlocking.\n");
    exit(0);
}