#include "binsem.h"

static


void binsem_init(sem_t *s, int init_val) {
    if (init_val == 0)
        xchg(s, 0);
    else
        xchg(s, 1);
}

void binsem_up(sem_t *s) {

}

int binsem_down(sem_t *s) {

    if (*s == 0) {
        // Blocking by exersice's definition
        alarm(0);
    }
    else if (*s > 0)
        xchg(s, s - 1);
}
