#include "ut.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

static ut_slot* threads_table;
static ucontext_t* uc_table;


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

    threads_table = malloc(tab_size * sizeof(ut_slot));
    uc_table = malloc(tab_size * sizeof(ucontext_t));

    return 0;
}

tid_t ut_spawn_thread(void (*func)(int), int arg) {

    ut_slot new_thread;

    // Checking if there is a room for a new thread to spawn
    if (threads_counter == threads_table_size) {
        printf("No more room for new threads\n");
        return TAB_FULL;
    }

    // Creating a new thread and set up it's values
    char new_thread_stack[STACKSIZE];

    if (new_thread_stack == NULL) {
        ("Couldn't allocate memory for thread #%d's stack\n", threads_counter);
        return SYS_ERR;
    }

    new_thread = malloc(sizeof(*new_thread));
    //printf("Size 1: %d\nSize 2: %d\nPointer: %p\n\n", sizeof(*new_thread), sizeof(new_thread), new_thread);

    if (new_thread == NULL) {
        printf("Couldn't allocate memory for thread #%d's table slot\n", threads_counter);
        return SYS_ERR;
    }


    ucontext_t* current_ucontext = &(threads_table[threads_counter]->uc);
    // Getting a new context for the new thread
    if (getcontext(current_ucontext) == -1) {
        printf("Fatal: Failed to get context for a new thread\n");
        return SYS_ERR;
    }

    // Setting up the new_ucontext stack
    current_ucontext->uc_stack.ss_sp = new_thread_stack;
    current_ucontext->uc_stack.ss_size = sizeof(new_thread_stack);
    current_ucontext->uc_stack.ss_flags = 0;

    // Setting up the context
    makecontext(current_ucontext, func, arg);

    // Setting up the new ut_slot
    new_thread->stack = new_thread_stack;
    new_thread->uc = *current_ucontext;
    new_thread->vtime = 0;
    new_thread->func = func;
    new_thread->arg = arg;

    // Saving the thread's number
    int current_thread_number = threads_counter;

    threads_table[threads_counter++] = new_thread;

    free(new_thread);

    return current_thread_number;
}

void handler(int signal) {

    printf("Signal: %d\n", signal);
    alarm(1);
    printf("thread: %d - time: %d\n", currThreadNum, ut_get_vtime(currThreadNum));

	printf("Curr: %d\n", currThreadNum);
	if (signal == SIGVTALRM){
		// update the vtime statistics
		printf("cell: %p\n", threads_table[currThreadNum]);
		threads_table[currThreadNum]->vtime += 100;
	}

	else {
        currThreadNum = ((currThreadNum + 1) % threads_counter);
        printf("SIGALRM\n");
        ucontext_t* nextThreadToRun = &(uc_table[0]);
        if (currThreadNum < (threads_counter - 1)) {
            nextThreadToRun = &(uc_table[currThreadNum + 1]);
        }
        swapcontext(&uc_table[currThreadNum], nextThreadToRun);
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
    itv.it_interval.tv_sec = 1;
    itv.it_interval.tv_usec = 0;
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
