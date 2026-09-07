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
    int burst_times[5] = {25, 5, 35, 15, 10};
    int pids[5];

    printf(1, "Starting SJF Scheduler Test...\n");

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();

        if (pids[i] == 0) {
            int my_pid = getpid();

            set_burst(my_pid, burst_times[i]);
            printf(1, "Child %d (PID: %d) declared burst_time = %d\n",
                   i + 1, my_pid, burst_times[i]);

            /*
             * Give the parent enough time to create the remaining
             * children so that SJF can compare all burst times.
             */
            sleep(50);

            cpu_bound_work();

            printf(1, "Child %d (PID: %d) finished!\n", i + 1, my_pid);
            exit();
        }
    }

    for (int i = 0; i < 5; i++) {
        wait();
    }

    printf(1, "SJF Test completed.\n");
    exit();
}
