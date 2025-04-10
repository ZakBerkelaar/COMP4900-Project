#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <stdatomic.h>
#include "benchmarks.h"
#include <string.h>

#define BENCHMARK_WORKERS 8
#define MAX_EVENTS_PER_QUEUE 64
#define TASK_CREATION_COUNT 255

typedef enum
{
    e_TaskArrived,
    e_TaskStarted,
    e_TaskFinished
} EventType;

typedef struct
{
    EventType type;
    Time_t time;

    union
    {
        struct
        {
            uint32_t worker_id;
        } arrived;

        struct
        {
            uint32_t worker_id;
        } started;

        struct
        {
            uint32_t worker_id;
        } finished;
    } data;
} Event;

typedef struct
{
    _Atomic uint32_t eventCount;
    Event events[MAX_EVENTS_PER_QUEUE];
} EventQueue;

typedef struct 
{
    uint32_t deadline;
    uint32_t runtime;
    uint32_t id;
    StaticTask_t tcb;
    EventQueue* queue;
} BenchmarkData;

static StackType_t watcher_stack[1024];
static StaticTask_t watcher_tcb;
static StackType_t worker_stacks[BENCHMARK_WORKERS][1024];
static BenchmarkData worker_data[BENCHMARK_WORKERS];
static StaticEventGroup_t event_storage;
static EventGroupHandle_t finished_event;

static EventQueue eQueue;

void push_event(Event* event)
{
    uint32_t idx = atomic_fetch_add(&eQueue.eventCount, 1);
    eQueue.events[idx] = *event;
}

static void benchmark_worker(void* data)
{
    Event e;
    BenchmarkData* bData = (BenchmarkData*)data;
    const uint32_t cycles = bData->runtime * 250000;
    printf("Task %d started\n", bData->id);

    e.type = e_TaskStarted;
    e.time = get_current_time();
    e.data.started.worker_id = bData->id;
    push_event(&e);

    volatile uint32_t dummy = 0;
    for(uint32_t i = 0; i < cycles; ++i)
    {
        dummy += i;
    }
    
    e.type = e_TaskFinished;
    e.time = get_current_time();
    e.data.finished.worker_id = bData->id;
    push_event(&e);

#if defined SCHED_LLREF
    TickType_t remaining = pubGetxRemainingExecutionTime(xTaskGetCurrentTaskHandle());
    printf("Task %d ended with %d remaining\n", bData->id, remaining);
#else
    printf("Task %d ended\n", bData->id);
#endif
    // Let the watcher know we are done here
    xEventGroupSetBits(finished_event, 1 << bData->id);

    vTaskDelete(xTaskGetCurrentTaskHandle());
}

#ifdef SCHED_EDF
static TickType_t deadlines[BENCHMARK_WORKERS] = 
{
    90,
    60,
    70,
    20,
    8,
    16,
    24,
    64
};
#endif

#if defined PLATFORM_RPI
static TickType_t execution_times[BENCHMARK_WORKERS] = 
{
    11802,
    18165,
    16107,
    18375,
    4977,
    14196,
    4137,
    18837
};
#elif defined PLATFORM_QEMU
static TickType_t execution_times[BENCHMARK_WORKERS] = 
{
    36,
    74,
    54,
    11,
    79,
    12,
    57,
    89
};
#endif

void create_benchmark_task(uint32_t id, uint32_t deadlineMs, uint32_t runtimeMs)
{
    BenchmarkData* data = &worker_data[id];
    char name[128];

    data->deadline = deadlineMs;
    data->runtime = runtimeMs;
    data->id = id;
    //data->queue = &worker_events[id];

    snprintf(name, sizeof(name), "BWorker%d", id);

    TaskHandle_t handle = xTaskCreateStatic(
        benchmark_worker,
        name,
        sizeof(worker_stacks[0]) / sizeof(worker_stacks[0][0]),
        data,
#if defined SCHED_EDF
        deadlines[id],
#elif defined SCHED_LLREF
        execution_times[id], 
#elif defined SCHED_DEFAULT
        configMAX_PRIORITIES - 1,
#else
        #error Unknown scheduler
#endif

        &worker_stacks[id][0],
        &worker_data[id].tcb
    );
}

void watcher(void* args)
{
    EventQueue* queues[BENCHMARK_WORKERS];
    uint32_t event_count;

    xEventGroupSync(finished_event, 0, (1 << BENCHMARK_WORKERS) - 1, portMAX_DELAY);
    printf("All tasks done\n");

    event_count = eQueue.eventCount;

    printf("%d context switches occured\n", get_context_switch_count());
    printf("Total events: %d\n", event_count);

    printf("---OUTPUT START---\n");
    for(uint32_t i = 0; i < event_count; ++i)
    {
        Event* current = &eQueue.events[i];
        uint32_t ms = current->time / get_time_frequency_ms();

        if(current->type == e_TaskArrived)
        {
            printf("%d ms | ARRIVE task%d\n", ms, current->data.finished.worker_id);
        }
        if(current->type == e_TaskStarted)
        {
            printf("%d ms | START task%d\n", ms, current->data.finished.worker_id);
        }
        else if(current->type == e_TaskFinished)
        {
            printf("%d ms | END task%d\n", ms, current->data.finished.worker_id);
        }
    }

    printf("----OUTPUT END----\n");
    app_abort();
}

StaticTask_t tempTaskTcbs[TASK_CREATION_COUNT];
TaskHandle_t tmpTaskHandles[TASK_CREATION_COUNT];

void task_creation_benchmark()
{
    printf("Starting\n");

    for(uint32_t j = 0; j < 2048 * 2048 * 64; ++j);
    {
        for(uint32_t i = 0; i < TASK_CREATION_COUNT; ++i)
        {
            tmpTaskHandles[i] = xTaskCreateStatic(
                NULL,
                "TestTask",
                1024,
                NULL,
                0,
                NULL,
                &tempTaskTcbs[i]
            );
        }

        for(uint32_t i = 0; i < TASK_CREATION_COUNT; ++i)
        {
            vTaskDelete(tmpTaskHandles[i]);
        }
    }

    printf("Ending %d\n", get_current_time() / get_time_frequency_ms());
    app_abort();
}

void run_benchmarks(void)
{
    Event e;

    finished_event = xEventGroupCreateStatic(&event_storage);
    if(!finished_event)
    {
        printf("Could not create event group\n");
        app_abort();
    }

    xTaskCreateStatic(
        watcher,
        "Watcher",
        sizeof(watcher_stack) / sizeof(watcher_stack[0]),
        NULL,
#if defined SCHED_LLREF
        (TickType_t)-1, // Big number so it runs first
#elif defined SCHED_EDF 
        0, // Deadline 0 to make it run first
#elif defined SCHED_DEFAULT
        0, // Priority 0
#else
        #error Unknown scheduler
#endif
        watcher_stack,
        &watcher_tcb
    );

    for(int i = 0; i < BENCHMARK_WORKERS; ++i)
    {
        // We can do about 21 cycles every tick on the rpi
        // We can do about 2 cycles every tick on qemu
        // These both assume no execessive logging

        #if defined PLATFORM_QEMU
        uint32_t divisor = 1;
        #elif defined PLATFORM_RPI
        uint32_t divisor = 19;
        #endif

        create_benchmark_task(i, 0, execution_times[i] / divisor);
    }

    // All tasks arrive at the same time
    e.type = e_TaskArrived;
    e.time = get_current_time();
    for(uint32_t i = 0; i < BENCHMARK_WORKERS; ++i)
    {
        e.data.arrived.worker_id = i;

        push_event(&e);
    }

    vTaskStartScheduler();

    printf("Benchmarks finished");
}
