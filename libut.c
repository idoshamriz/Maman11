#include "ut.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

static ut_slot* threads_table;

static unsigned int threads_counter = 0;
static ucontext_t mainThread;
static int threads_table_size = 0;
static volatile tid_t currThreadNum;

int ut_init(int tab_size) {
    if (tab_size < MIN_TAB_SIZE || tab_size > MAX_TAB_SIZE) {
        printf("Info: the table size is not in the valide range. Setting table size to be %d\n", MAX_TAB_SIZE);
        tab_size = MAX_TAB_SIZE;
    }

    threads_table_size = tab_size;
    threads_table = (ut_slot*)malloc(tab_size * sizeof(ut_slot));

    return 0;
}

tid_t ut_spawn_thread(void (*func)(int), int arg) {

    ut_slot new_thread;
    ucontext_t* current_ucontext;
    char new_thread_stack[STACKSIZE];
    tid_t current_thread_number = threads_counter;

    // Checking if there is a room for a new thread to spawn
    if (threads_counter == threads_table_size) {
        printf("No more room for new threads\n");
        return TAB_FULL;
    }

    threads_table[threads_counter] = (ut_slot)malloc(sizeof(ut_slot_t));

    current_ucontext = malloc(sizeof(ucontext_t));
    printf("current ucontext: %p\n", current_ucontext);
    // Getting a new context for the new thread
    if (getcontext(current_ucontext) == -1) {
        printf("Fatal: Failed to get context for a new thread\n");
        return SYS_ERR;
    }

    // Setting up the new_ucontext stack
    current_ucontext->uc_stack.ss_sp = new_thread_stack;
    current_ucontext->uc_stack.ss_size = sizeof(new_thread_stack);
    current_ucontext->uc_stack.ss_flags = 0;
    current_ucontext->uc_link = &mainThread;

    // Setting up the context
    makecontext(current_ucontext, func, arg);

    // Setting up the new ut_slot
    threads_table[threads_counter]->stack = new_thread_stack;
    threads_table[threads_counter]->uc = *current_ucontext;
    threads_table[threads_counter]->vtime = 0;
    threads_table[threads_counter]->func = func;
    threads_table[threads_counter]->arg = arg;

   // printf("new thread: %p\n", new_thread);
    printf("Inserted to %p (thread counter: %d)\n", threads_table[threads_counter++], threads_counter);

    free(new_thread);
    return current_thread_number;
}

void handler(int signal) {
    printf("thread: %d - time: %d - cell: %p\n\n", currThreadNum, ut_get_vtime(currThreadNum), &threads_table[currThreadNum]);

	switch (signal){
        case SIGALRM:
            alarm(1);
            currThreadNum = ((currThreadNum + 1) % threads_counter);
            printf("SIGALRM\n");
            ucontext_t* nextThreadToRun = &(threads_table[0]->uc);
            if (currThreadNum < (threads_counter - 1)) {
                nextThreadToRun = &(threads_table[currThreadNum + 1]->uc);
            }
            swapcontext(&threads_table[currThreadNum]->uc, nextThreadToRun);
        break;
        case SIGVTALRM:
            threads_table[currThreadNum]->vtime += 10;
        break;
        default:
        break;
	}
}

int ut_start(void) {
    /*if (set_threads_ulinks() != 0) {
        printf("Failed to set order between threads' switching");
        return SYS_ERR;
    }*/

    int current_thread_index = 0;

    struct sigaction sa;
    struct itimerval itv;

    // Setting up the signal action struct
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);
    sa.sa_handler = handler;

    // Setting up the timer interval struct
    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 10000;
    itv.it_value = itv.it_interval;

    if (sigaction(SIGALRM, &sa, NULL) < 0)
		return SYS_ERR;

    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
		return SYS_ERR;

	if (setitimer(ITIMER_VIRTUAL, &itv, NULL) < 0)
		return SYS_ERR;

    alarm(1);
    currThreadNum = 0;
	while (1);
    return 0;
}

unsigned long ut_get_vtime(tid_t tid) {
    if (tid > threads_counter) {
        printf("Thread does not exist!\n");
        return SYS_ERR;
    }
    return threads_table[tid]->vtime;
}
