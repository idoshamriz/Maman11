#include "ut.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/time.h>

static ut_slot* threads_table;
static unsigned int threads_counter = 0;
static int threads_table_size = 0;
static volatile tid_t currThreadNum;

int ut_init(int tab_size) {
    if (tab_size < MIN_TAB_SIZE || tab_size > MAX_TAB_SIZE) {
        printf("Info: the table size is not in the valide range. Setting table size to be %d\n", MAX_TAB_SIZE);
        tab_size = MAX_TAB_SIZE;
    }

    threads_table_size = tab_size;
    threads_table = malloc(tab_size * sizeof(ut_slot));
    return 0;
}

tid_t ut_spawn_thread(void (*func)(int), int arg) {

    ucontext_t new_ucontext;
    ut_slot new_thread;

    // Checking if there is a room for a new thread to spawn
    if (threads_counter == threads_table_size) {
        printf("No more room for new threads\n");
        return TAB_FULL;
    }

    // Creating a new thread and set up it's values
    char new_thread_stack[STACKSIZE];

    if (new_thread_stack == NULL) {
        printf("Couldn't allocate memory for thread #%d's stack\n", threads_counter);
        return SYS_ERR;
    }

    new_thread = malloc(sizeof(*new_thread));

    if (new_thread == NULL) {
        printf("Couldn't allocate memory for thread #%d's table slot\n", threads_counter);
        return SYS_ERR;
    }

    // Getting a new context for the new thread
    if (getcontext(&new_ucontext) == -1) {
        printf("Fatal: Failed to get context for a new thread\n");
        return SYS_ERR;
    }

    // Setting up the new_ucontext stack
    new_ucontext.uc_stack.ss_sp = new_thread_stack;
    new_ucontext.uc_stack.ss_size = sizeof(new_thread_stack);
    new_ucontext.uc_stack.ss_flags = 0;

    // Setting up the context
    makecontext(&new_ucontext, func, arg);

    // Setting up the new ut_slot
    new_thread->stack = new_thread_stack;
    new_thread->uc = new_ucontext;
    new_thread->func = func;
    new_thread->arg = arg;
    new_thread->vtime = 0;

    // Saving the thread's number
    int current_thread_number = threads_counter;
    threads_table[threads_counter++] = &new_thread;

    free(new_thread);

    return current_thread_number;
}

void handler(int signal) {
    alarm(1);
    currThreadNum = (currThreadNum % threads_counter);

    ut_slot current_thread = threads_table[currThreadNum];

    printf("Current thread number: %d\n", currThreadNum);

    printf("Thread Time: %d\n", ut_get_vtime(currThreadNum));

	if (signal == SIGVTALRM){
		// update the vtime statistics
		printf("Receive sigvtalram\n");
		threads_table[currThreadNum]->vtime += 1;
    //    printf("\nvtime %ld sec.\n", ut_get_vtime(currThreadNum), ut_get_vtime(currThreadNum));
	}
	else if (signal == SIGALRM) {
        printf("SIGALRM\n");
        swapcontext(&(threads_table[currThreadNum]->uc), threads_table[currThreadNum]->uc.uc_link);
        //printf("%p -> %p\n", &(threads_table[currThreadNum]->uc), threads_table[currThreadNum]->uc.uc_link);
        currThreadNum++;
	}
}

/* Sets each thread to switch it's context to the next thread on the table.
   In case of 1 thread only, it will switch to itself */
int set_threads_ulinks() {

    // If there aren't any threads
    if (threads_counter == 0) {
        printf("No threads created\n");
        return SYS_ERR;
    }

    // Set each thread's link to the following thread
    int i = 0;
    for (i; i < threads_counter; i++) {
        printf("%p -> %p\n", &(threads_table[i]->uc), &threads_table[i + 1]->uc);
        threads_table[i]->uc.uc_link = &threads_table[i + 1]->uc;
    }

    // Set the last thread's link to the first thread
    threads_table[threads_counter - 1]->uc.uc_link = &(threads_table[0]->uc);

    return 0;
}

int ut_start(void) {

    if (set_threads_ulinks() != 0) {
        printf("Failed to set order between threads' switching");
        return SYS_ERR;
    }

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

    if (sigaction(SIGVTALRM, &sa, NULL) < 0)
		return SYS_ERR;

	if (setitimer(ITIMER_VIRTUAL, &itv, NULL) < 0)
		return SYS_ERR;

	if (sigaction(SIGALRM, &sa, NULL) < 0)
		return SYS_ERR;

    currThreadNum = 0;


	while (1) {
	}

    return 0;
}

unsigned long ut_get_vtime(tid_t tid) {
    if (tid > threads_counter) {
        printf("Thread does not exist!\n");
        return SYS_ERR;
    }
    printf("TID: %d", tid);
    return threads_table[tid]->vtime;
}
