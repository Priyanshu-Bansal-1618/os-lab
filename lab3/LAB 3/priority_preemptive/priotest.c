#include "types.h"
#include "stat.h"
#include "user.h"

void cpu_work(void) {
    volatile double x = 0;

    for (int i = 0; i < 50000000; i++) {
        x += 3.14 * 2.71;
    }
}

int main(void) {
    int priorities[5] = {18, 4, 12, 7, 15};
    int pids[5];

    printf(1, "Starting Priority Scheduler Test...\n");

    for (int i = 0; i < 5; i++) {
        pids[i] = fork();

        if (pids[i] == 0) {
            int my_pid = getpid();

            set_priority(my_pid, priorities[i]);

            printf(1, "Child %d (PID: %d) started with priority %d\n",
                   i + 1, my_pid, priorities[i]);

            cpu_work();

            printf(1, "Child %d (PID: %d) finished!\n", i + 1, my_pid);
            exit();
        }
    }

    for (int i = 0; i < 5; i++) {
        wait();
    }

    printf(1, "Priority Scheduler Test completed.\n");
    exit();
}
