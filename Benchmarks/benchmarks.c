#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include "benchmarks.h"

#define BENCHMARK_WORKERS 8
#define MAX_EVENTS_PER_QUEUE 64

typedef enum
{
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
        } finished;
    } data;
} Event;

typedef struct
{
    uint32_t eventCount;
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
static EventQueue worker_events[BENCHMARK_WORKERS];
static uint32_t worker_count = 0;
static StaticEventGroup_t event_storage;
static EventGroupHandle_t finished_event;

void push_event(EventQueue* queue, Event* event)
{
    if(queue->eventCount >= MAX_EVENTS_PER_QUEUE)
    {
        printf("Too many events in queue\n");
        return;
    }

    queue->events[queue->eventCount++] = *event;
}

uint32_t combine_queues(EventQueue** queues, uint32_t numQueues, Event** output, uint32_t numOut)
{
    uint32_t outCounter = 0;
    uint32_t inCounter[numQueues];
    Event* earliest = NULL;
    uint32_t earliest_queue;
    uint32_t total = 0;

    for(uint32_t i = 0; i < numQueues; ++i)
    {
        total += queues[i]->eventCount;
    }

    if(total > numOut)
    {
        printf("Too many events in input to combine into output\n");
        return 0;
    }

    for(uint32_t i = 0; i < numQueues; ++i)
    {
        inCounter[i] = 0;
    }

    while(1)
    {
        earliest = NULL;
        for(uint32_t i = 0; i < numQueues; ++i)
        {
            if(inCounter[i] < queues[i]->eventCount &&
               (!earliest || queues[i]->events[inCounter[i]].time < earliest->time))
            {
                earliest = &queues[i]->events[inCounter[i]];
                earliest_queue = i;
            }
        }
        if(!earliest)
        {
            // No data left
            break;
        }

        output[outCounter++] = earliest;
        inCounter[earliest_queue]++;
    }

    return outCounter;
}

static void benchmark_worker(void* data)
{
    Event e;
    BenchmarkData* bData = (BenchmarkData*)data;
    printf("Starting\n");

    const uint32_t cycles = bData->runtime * 250000;

    volatile uint8_t dummy = 0;
    for(uint32_t i; i < cycles; ++i)
    {
        dummy += (uint8_t)i;
    }

    e.type = e_TaskFinished;
    e.time = get_current_time();
    e.data.finished.worker_id = bData->id;
    push_event(bData->queue, &e);

    xEventGroupSetBits(finished_event, 1 << bData->id);
    printf("Done\n");

    vTaskDelete(xTaskGetCurrentTaskHandle());
}

void create_benchmark_task(uint32_t id, uint32_t deadlineMs, uint32_t runtimeMs)
{
    BenchmarkData* data = &worker_data[worker_count];
    char name[128];

    data->deadline = deadlineMs;
    data->runtime = runtimeMs;
    data->id = id;
    data->queue = &worker_events[worker_count];

    snprintf(name, sizeof(name), "BenchmarkWorker%d", worker_count);

    xTaskCreateStatic(
        benchmark_worker,
        name,
        sizeof(worker_stacks[0]) / sizeof(worker_stacks[0][0]),
        data,
        4,
        &worker_stacks[worker_count][0],
        &worker_data[worker_count].tcb
    );

    worker_count++;
}

static Event* output_events_sorted[128];

void watcher(void* args)
{
    EventQueue* queues[BENCHMARK_WORKERS];
    uint32_t event_count;

    xEventGroupSync(finished_event, 0, (1 << BENCHMARK_WORKERS) - 1, portMAX_DELAY);
    printf("All tasks done\n");

    for(uint32_t i = 0; i < BENCHMARK_WORKERS; ++i)
        queues[i] = &worker_events[i];

    event_count = combine_queues(queues, BENCHMARK_WORKERS, output_events_sorted, sizeof(output_events_sorted) / sizeof(output_events_sorted[0]));

    printf("Total events: %d\n", queues[0]->eventCount);

    printf("---OUTPUT START---\n");
    for(uint32_t i = 0; i < event_count; ++i)
    {
        Event* current = output_events_sorted[i];
        uint32_t ms = current->time / get_time_frequency_ms();

        if(current->type == e_TaskFinished)
        {
            printf("Task %d finished at time %d ms\n", current->data.finished.worker_id, ms);
        }
    }

    printf("----OUTPUT END----\n");
    app_abort();
}

void run_benchmarks(void)
{
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
        4,
        watcher_stack,
        &watcher_tcb
    );

    for(int i = 0; i < BENCHMARK_WORKERS; ++i)
    {
        create_benchmark_task(i, 0, 2000);
    }

    vTaskStartScheduler();

    printf("Benchmarks finished");
}