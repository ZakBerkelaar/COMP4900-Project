#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semihosting.h"
#include "benchmarks.h"

void test_task(void* params)
{
    printf("We are running the test task\n");

    uint32_t tick = semihosting_tickfreq();

    printf("The tick freq: %d\n", tick);

    uint32_t i = 0;
    volatile unsigned char tmp = 0;
    while(i < 4000000000) 
    {
        tmp += (unsigned char)i;

        ++i;
    }

    printf("TmpEnd %s\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));

    configRUN_TIME_COUNTER_TYPE elapsed = ulTaskGetRunTimeCounter(xTaskGetCurrentTaskHandle());

    uint64_t tmptime = ((uint64_t)elapsed) * 1000000;

    uint64_t secs = tmptime / tick;

    printf("This took %d seconds\n", (uint32_t)secs);

    vTaskDelete(xTaskGetCurrentTaskHandle());
}

void app_main(void)
{
#ifdef SCHED_LLREF
    printf("We are running llref\n");
#else
    printf("We are running edf\n");
#endif

    run_benchmarks();

    vTaskStartScheduler();

    printf("FreeRTOS scheduler exited. This is bad\n");

    while(1) {}
}

void print_new_task(void)
{
    TaskHandle_t newTask = xTaskGetCurrentTaskHandle();
    const char* name = pcTaskGetName(newTask);

    printf("Switching in %s\n", name);
}