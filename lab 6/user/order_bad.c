#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Helper functions to use a pipe as a blocking mutex
void lock_init(int p[2]) {
    pipe(p);
    write(p[1], "x", 1); // Write 1 byte to make it "available"
}

void lock_acquire(int p[2]) {
    char c;
    read(p[0], &c, 1); // Blocks if the pipe is empty
}

void lock_release(int p[2]) {
    write(p[1], "x", 1); // Puts the byte back
}

int main() {
    int lock1[2], lock2[2];
    
    // Initialize our two OS-level locks
    lock_init(lock1);
    lock_init(lock2);

    printf("\n=== Running DEADLOCK Scenario ===\n");

    int pidA = fork();
    if (pidA == 0) {
        // PROCESS A
        lock_acquire(lock1);
        printf("[Process A] Acquired Lock 1\n");
        
        sleep(10); // Deliberate delay to ensure context switch
        
        printf("[Process A] Waiting for Lock 2...\n");
        lock_acquire(lock2); // WILL HANG HERE
        printf("[Process A] Acquired Lock 2! (You should not see this)\n");
        
        lock_release(lock2);
        lock_release(lock1);
        exit(0);
    }

    int pidB = fork();
    if (pidB == 0) {
        // PROCESS B
        lock_acquire(lock2);
        printf("[Process B] Acquired Lock 2\n");
        
        sleep(10); // Deliberate delay to ensure context switch
        
        printf("[Process B] Waiting for Lock 1...\n");
        lock_acquire(lock1); // WILL HANG HERE
        printf("[Process B] Acquired Lock 1! (You should not see this)\n");
        
        lock_release(lock1);
        lock_release(lock2);
        exit(0);
    }

    // Parent waits for children (will hang forever)
    wait(0);
    wait(0);
    
    printf("All processes finished.\n");
    exit(0);
}