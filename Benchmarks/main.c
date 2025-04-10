#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semihosting.h"
#ifdef PLATFORM_RPI
#include "pico/stdlib.h"
#endif

extern void app_main(void);

/* printf() output uses the UART.  These constants define the addresses of the
 * required UART registers. */
#define UART0_ADDRESS                         ( 0x40004000UL )
#define UART0_DATA                            ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 0UL ) ) ) )
#define UART0_STATE                           ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 4UL ) ) ) )
#define UART0_CTRL                            ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 8UL ) ) ) )
#define UART0_BAUDDIV                         ( *( ( ( volatile uint32_t * ) ( UART0_ADDRESS + 16UL ) ) ) )
#define TX_BUFFER_MASK                        ( 1UL )

static void prvUARTInit( void )
{
    UART0_BAUDDIV = 16;
    UART0_CTRL = 1;
}

void main(void)
{
#if defined PLATFORM_QEMU
    prvUARTInit();
    printf("Running on qemu\n");
#elif defined PLATFORM_RPI
    stdio_init_all();
    getchar(); // Wait until we receive something from serial so we can capture the entire trace
    #if defined USE_SMP
        printf("Running on rpi using SMP (%d cores)\n", configNUMBER_OF_CORES);
    #else
        printf("Running on rpi without SMP\n");
    #endif
#endif
    app_main();
    return;
}

void vAssertCalled(const char *pcFileName, uint32_t ulLine)
{
    return;
}

void vApplicationMallocFailedHook(void)
{
    return;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
}

#ifdef USE_SMP
void vApplicationGetPassiveIdleTaskMemory(StaticTask_t ** ppxIdleTaskTCBBuffer,
                                          StackType_t ** ppxIdleTaskStackBuffer,
                                          configSTACK_DEPTH_TYPE * puxIdleTaskStackSize,
                                          BaseType_t xPassiveIdleTaskIndex )
{
    // There are configNUMBER_OF_CORES - 1 PASSIVE idle tasks
    static StaticTask_t passiveIdleTCBs[configNUMBER_OF_CORES - 1];
    static StackType_t passiveIdleStacks[configNUMBER_OF_CORES - 1][configMINIMAL_STACK_SIZE];

    *ppxIdleTaskTCBBuffer = &passiveIdleTCBs[xPassiveIdleTaskIndex];
    *ppxIdleTaskStackBuffer = &passiveIdleStacks[xPassiveIdleTaskIndex][0];
    *puxIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
#endif

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}


/*-----------------------------------------------------------*/

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

configRUN_TIME_COUNTER_TYPE get_runtime_counter(void)
{
    return get_current_time();
}

void app_abort(void)
{
#if defined PLATFORM_QEMU
    semihosting_exit();
#elif defined PLATFORM_RPI
    printf("App aborted\n");
    while(1) {}
#else
    #error Not implemented
#endif
}

#define CLOCK_DIVISOR 100000

Time_t get_current_time(void)
{
#if defined PLATFORM_QEMU
    return (semihosting_elapsed() / CLOCK_DIVISOR);
#elif defined PLATFORM_RPI
    absolute_time_t time = get_absolute_time();
    return to_ms_since_boot(time);
#else
    #error Not implemented
#endif
}

Time_t get_time_frequency_ms(void)
{
#if defined PLATFORM_QEMU
    return semihosting_tickfreq() / (1000 * CLOCK_DIVISOR);
#elif defined PLATFORM_RPI
    return 1; // It's already in ms on rpi
#else
    #error Not implemented
#endif
}

static uint32_t context_switch_count = 0;

void print_new_task(void)
{
#if defined USE_SMP
    static TaskHandle_t previous_tasks[configNUMBER_OF_CORES];
    for(uint32_t i = 0; i < configNUMBER_OF_CORES; ++i)
    {
        TaskHandle_t task = xTaskGetCurrentTaskHandleForCore(i);

        if(previous_tasks[i] != task)
        {
            printf("%d | Core%d: %s\n", xTaskGetTickCount(), i, pcTaskGetName(task));
            previous_tasks[i] = task;
        }
    }
#else
    TaskHandle_t newTask = xTaskGetCurrentTaskHandle();
    const char* name = pcTaskGetName(newTask);

    printf("%d | Core0: %s\n", xTaskGetTickCount(), name);
#endif
    context_switch_count++;
}

uint32_t get_context_switch_count()
{
    return context_switch_count;
}