#include "types.h"
#include "stat.h"
#include "user.h"

void cpu_bound_work(void) {
    volatile double x = 0;

    for (int i = 0; i < 100000000; i++) {
        x += 3.14 * 2.71;
    }
}

int main(void) {
    int n = 4;
    int pid;

    printf(1, "Starting FCFS Scheduler Test...\n");

    for (int i = 0; i < n; i++) {
        pid = fork();

        if (pid == 0) {
            printf(1, "Child %d (PID: %d) created\n", i + 1, getpid());

            cpu_bound_work();

            printf(1, "Child %d (PID: %d) finished\n", i + 1, getpid());
            exit();
        } else if (pid < 0) {
            printf(1, "Fork failed!\n");
            exit();
        }

        /* Give each child a different arrival time in the process table. */
        sleep(5);
    }

    for (int i = 0; i < n; i++) {
        wait();
    }

    printf(1, "FCFS Scheduler Test completed.\n");
    exit();
}
