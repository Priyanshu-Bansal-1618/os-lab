#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// --- Counting Semaphore Implementation using Pipes ---
void sem_init(int p[2], int count) {
    pipe(p);
    for(int i = 0; i < count; i++) {
        write(p[1], "x", 1); // Add 'count' available instances
    }
}

void sem_wait(int p[2]) {
    char c;
    read(p[0], &c, 1); // Claim 1 instance (blocks if 0 available)
}

void sem_post(int p[2]) {
    write(p[1], "x", 1); // Release 1 instance back to the pool
}

// Global Resource Pools (Semaphores)
int scanner[2]; // 1 instance available
int printer[2]; // 2 instances available
int disk[2];    // 2 instances available

// The Worker Function for all processes
// Enforces Resource Ordering: r1 must strictly be a lower-order resource than r2
void p_work(int id, int r1[2], char* r1_name, int r2[2], char* r2_name) {
    for (int cycle = 1; cycle <= 2; cycle++) {
        printf("[P%d - Cycle %d] Requesting %s...\n", id, cycle, r1_name);
        sem_wait(r1);
        printf("[P%d - Cycle %d] GRANTED %s. Requesting %s...\n", id, cycle, r1_name, r2_name);
        
        sem_wait(r2);
        printf("[P%d - Cycle %d] GRANTED %s. Starting work.\n", id, cycle, r2_name);
        
        sleep(5); // Simulate working with the hardware
        
        printf("[P%d - Cycle %d] Work done. Releasing %s and %s.\n", id, cycle, r2_name, r1_name);
        sem_post(r2);
        sem_post(r1);
        
        sleep(2); // Short break before the next cycle
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    // Initialize Resource instances
    sem_init(scanner, 1);
    sem_init(printer, 2);
    sem_init(disk, 2);

    printf("\n=== Multi-Resource Sync & Deadlock Prevention ===\n");
    printf("Strategy: Resource Ordering (Order: Scanner -> Printer -> Disk)\n\n");

    // Create 5 Processes needing 2 out of 3 resources.
    // Notice that the arguments passed ALWAYS follow the global order.
    // e.g., P1 needs Printer and Disk, so it asks for Printer first.
    
    if (fork() == 0) p_work(0, scanner, "Scanner", printer, "Printer");
    if (fork() == 0) p_work(1, printer, "Printer", disk, "Disk");
    if (fork() == 0) p_work(2, scanner, "Scanner", disk, "Disk");
    if (fork() == 0) p_work(3, scanner, "Scanner", printer, "Printer");
    if (fork() == 0) p_work(4, printer, "Printer", disk, "Disk");

    // Parent waits for all 5 children to finish their 2 cycles
    for(int i = 0; i < 5; i++) {
        wait(0);
    }

    printf("\nSUCCESS: All 5 processes completed multiple cycles without hanging.\n");
    exit(0);
}