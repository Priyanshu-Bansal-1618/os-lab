#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define P 4 // Number of processes (P0 to P3)
#define R 4 // Number of resources (R0 to R3)

int alloc[P][R];
int req[P][R];
int wfg[P][P]; // Wait-For Graph adjacency matrix

// Variables for DFS cycle detection
int visited[P];
int rec_stack[P];
int parent[P];

// Resets matrices for different scenarios
void reset_matrices() {
    for(int i = 0; i < P; i++) {
        visited[i] = 0;
        rec_stack[i] = 0;
        parent[i] = -1;
        for(int j = 0; j < R; j++) {
            alloc[i][j] = 0;
            req[i][j] = 0;
        }
        for(int j = 0; j < P; j++) {
            wfg[i][j] = 0;
        }
    }
}

// Builds Wait-For Graph: Edge Pi -> Pj exists if Pi requests a resource held by Pj
void build_wfg() {
    for (int i = 0; i < P; i++) {          // Process i making a request
        for (int k = 0; k < R; k++) {      // For resource k
            if (req[i][k] > 0) {           // If Pi requests Rk
                for (int j = 0; j < P; j++) { // Check who holds Rk
                    if (alloc[j][k] > 0 && i != j) {
                        wfg[i][j] = 1;     // Pi is waiting on Pj
                    }
                }
            }
        }
    }
}

// Prints the Wait-For Graph
void print_wfg() {
    printf("Constructed Wait-For Graph Edges:\n");
    int edges = 0;
    for (int i = 0; i < P; i++) {
        for (int j = 0; j < P; j++) {
            if (wfg[i][j]) {
                printf("  P%d -> P%d (P%d is waiting for P%d)\n", i, j, i, j);
                edges++;
            }
        }
    }
    if (edges == 0) printf("  [No edges - no processes are waiting]\n");
}

// DFS to detect cycles in the Wait-For Graph
int detect_cycle(int v) {
    visited[v] = 1;
    rec_stack[v] = 1;

    for (int i = 0; i < P; i++) {
        if (wfg[v][i]) { // If there is an edge from v to i
            if (!visited[i]) {
                parent[i] = v;
                if (detect_cycle(i)) return 1;
            } else if (rec_stack[i]) {
                // Cycle detected! Print the path.
                printf("\n=> DEADLOCK DETECTED!\n");
                printf("=> Circular Wait Cycle: ");
                
                int curr = v;
                int cycle_nodes[P + 2];
                int c_idx = 0;
                
                cycle_nodes[c_idx++] = i; // End of cycle
                while (curr != i) {
                    cycle_nodes[c_idx++] = curr;
                    curr = parent[curr];
                }
                cycle_nodes[c_idx++] = i; // Start of cycle
                
                // Print array in reverse order to show proper flow
                for (int k = c_idx - 1; k >= 0; k--) {
                    printf("P%d", cycle_nodes[k]);
                    if (k != 0) printf(" -> ");
                }
                printf("\n");
                return 1;
            }
        }
    }
    rec_stack[v] = 0; // Remove from recursion stack
    return 0;
}

void run_detection(const char* scenario_name) {
    printf("\n=== %s ===\n", scenario_name);
    build_wfg();
    print_wfg();

    int deadlocked = 0;
    for (int i = 0; i < P; i++) {
        if (!visited[i]) {
            if (detect_cycle(i)) {
                deadlocked = 1;
                break;
            }
        }
    }

    if (!deadlocked) {
        printf("\n=> RESULT: No deadlock detected. Wait-for graph is acyclic.\n");
    }
    printf("-------------------------------------------\n");
}

int main(int argc, char *argv[]) {
    // SCENARIO 1: No Deadlock (Acyclic)
    reset_matrices();
    // Allocations
    alloc[0][0] = 1; // P0 holds R0
    alloc[1][1] = 1; // P1 holds R1
    alloc[2][2] = 1; // P2 holds R2
    
    // Requests
    req[1][0] = 1;   // P1 requests R0 (held by P0)
    req[2][1] = 1;   // P2 requests R1 (held by P1)
    
    // Expected Graph: P2 -> P1 -> P0 (No cycle)
    run_detection("Scenario 1: No Deadlock");

    // SCENARIO 2: Deadlock with a 3-process cycle
    reset_matrices();
    // Allocations
    alloc[0][0] = 1; // P0 holds R0
    alloc[1][1] = 1; // P1 holds R1
    alloc[2][2] = 1; // P2 holds R2
    
    // Requests causing a circular wait
    req[0][1] = 1;   // P0 requests R1 (held by P1)
    req[1][2] = 1;   // P1 requests R2 (held by P2)
    req[2][0] = 1;   // P2 requests R0 (held by P0)
    
    // Expected Graph: P0 -> P1 -> P2 -> P0 (Cycle!)
    run_detection("Scenario 2: Deadlock (3-Process Cycle)");

    exit(0);
}