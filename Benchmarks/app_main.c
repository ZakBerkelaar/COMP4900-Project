#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semihosting.h"
#include "benchmarks.h"

void app_main(void)
{
#if defined SCHED_LLREF
    printf("We are running llref\n");
#elif defined SCHED_EDF
    printf("We are running edf\n");
#elif defined SCHED_DEFAULT
    printf("We are running default\n");
#else
    #error Unknown scheduler
#endif

    run_benchmarks();

    vTaskStartScheduler();

    printf("FreeRTOS scheduler exited. This is bad\n");

    while(1) {}
}