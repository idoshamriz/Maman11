#include "binsem.h"

#define LOCKED 0
#define UNLOCKED 1
#define PID_ERROR -1


void binsem_init(sem_t *s, int init_val) {
    if (s == NULL) {
        fprintf(stderr, "Error: invalid value for semaphore init");
        exit(SYS_ERR);
    }

    // Setting the semaphore's initial value based on the received input
    sem_t sem_init_value = (LOCKED == init_val) ? LOCKED : UNLOCKED;
    xchg(s, sem_init_value)
}

void binsem_up(sem_t *s) {
    // Informing that the critical section can be entered
    xchg(s, UNLOCKED);
}

int binsem_down(sem_t *s) {
    pid_t current_process_id = NULL;
    sem_t use_semphore = LOCKED;

    // "Blocking" the process until the semaphore is released
    while (LOCKED == use_semphore) {

        // Trying to lock the semaphore to the current thread
        use_semphore = xchg(s, LOCKED);

        // If another thread already locked the semaphor, "block" the current one (by sending an alarm and
        // run the following thread
        if (LOCKED == lock_semphore) {
            current_process_id = getpid();

            if (PID_ERROR == kill(current_process_id, SIGALRM)) {
                fprintf(stderr, "Failed to block current thread");
                return SYS_ERR;
            }
        }
    }

    // Exiting the while loop states that the current thread managed to LOCK an UNLOCKED semaphore
    // atomically, which means it is currently running

    return 0;
}
