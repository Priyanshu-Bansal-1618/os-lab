#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define P 5 // Number of processes
#define R 3 // Number of resource types

// Global State
int alloc[P][R] = {
    {0, 1, 0}, // P0
    {2, 0, 0}, // P1
    {3, 0, 2}, // P2
    {2, 1, 1}, // P3
    {0, 0, 2}  // P4
};

int max[P][R] = {
    {7, 5, 3}, // P0
    {3, 2, 2}, // P1
    {9, 0, 2}, // P2
    {2, 2, 2}, // P3
    {4, 3, 3}  // P4
};

int avail[R] = {3, 3, 2};
int need[P][R];

// Compute Need matrix: Need[i][j] = Max[i][j] - Alloc[i][j]
void calc_need() {
    for (int i = 0; i < P; i++) {
        for (int j = 0; j < R; j++) {
            need[i][j] = max[i][j] - alloc[i][j];
        }
    }
}

// Safety Algorithm
// Returns 1 if safe, 0 if unsafe.
int check_safe(int print_seq) {
    int work[R];
    int finish[P];
    int safe_seq[P];
    int count = 0;

    for (int i = 0; i < R; i++) work[i] = avail[i];
    for (int i = 0; i < P; i++) finish[i] = 0;

    while (count < P) {
        int found = 0;
        for (int p = 0; p < P; p++) {
            if (finish[p] == 0) {
                int can_finish = 1;
                for (int j = 0; j < R; j++) {
                    if (need[p][j] > work[j]) {
                        can_finish = 0;
                        break;
                    }
                }
                
                if (can_finish) {
                    for (int k = 0; k < R; k++) {
                        work[k] += alloc[p][k];
                    }
                    safe_seq[count++] = p;
                    finish[p] = 1;
                    found = 1;
                }
            }
        }
        if (found == 0) {
            // System is not in a safe state
            return 0;
        }
    }

    if (print_seq) {
        printf("System is in a SAFE state.\nSafe Sequence: ");
        for (int i = 0; i < P; i++) {
            printf("P%d ", safe_seq[i]);
            if (i != P - 1) printf("-> ");
        }
        printf("\n");
    }
    return 1;
}

// Resource Request Algorithm
void request_resources(int pid, int req[R]) {
    printf("\n--- Process P%d requesting resources: (%d, %d, %d) ---\n", pid, req[0], req[1], req[2]);
    
    for (int i = 0; i < R; i++) {
        if (req[i] > need[pid][i]) {
            printf("Error: Request exceeds maximum need.\n");
            return;
        }
        if (req[i] > avail[i]) {
            printf("Denied: Resources not available right now. P%d must wait.\n", pid);
            return;
        }
    }

    // Pretend to allocate
    for (int i = 0; i < R; i++) {
        avail[i] -= req[i];
        alloc[pid][i] += req[i];
        need[pid][i] -= req[i];
    }

    // Check if new state is safe
    if (check_safe(1)) {
        printf("Request GRANTED.\n");
    } else {
        printf("Request DENIED - would lead to unsafe state. Rolling back.\n");
        // Rollback
        for (int i = 0; i < R; i++) {
            avail[i] += req[i];
            alloc[pid][i] -= req[i];
            need[pid][i] += req[i];
        }
    }
}

int main(int argc, char *argv[]) {
    printf("\n=== Banker's Algorithm Simulation ===\n");
    calc_need();
    
    printf("\nInitial Safety Check:\n");
    check_safe(1);

    // Scenario 1: Granted Safely
    // P1 requests 1 instance of A, 0 of B, 2 of C
    int req1[] = {1, 0, 2};
    request_resources(1, req1);

    // Scenario 2: Denied due to Unsafe State
    // P0 requests 0 of A, 2 of B, 0 of C. 
    // This is within available limits, but causes deadlock (Unsafe)
    int req2[] = {0, 2, 0};
    request_resources(0, req2);

    exit(0);
}