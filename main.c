#include <stdio.h>
#include <stdlib.h>
#include "ut.h"

void printArg(int arg) {
    printf("In thread number %d\n", arg);
}

int main()
{
    if(ut_init(10) == 0) {
        int thread_0 = ut_spawn_thread(printArg, 0);
        int thread_1 = ut_spawn_thread(printArg, 1);
        int thread_2 = ut_spawn_thread(printArg, 2);
        int thread_3 = ut_spawn_thread(printArg, 3);
        //int thread_5 = ut_spawn_thread(printArg, 3);
        //int thread_6 = ut_spawn_thread(printArg, 3); */

        printf("%d\n", ut_start());
    }

    return 0;
}
