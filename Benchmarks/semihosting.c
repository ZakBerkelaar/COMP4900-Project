#include "semihosting.h"
#include <stdio.h>

#define SYS_CLOCK (0x10)
#define SYS_EXIT (0x18)
#define SYS_ELAPSED (0x30)
#define SYS_TICKFREQ (0x31)

static uint32_t semihost_call(uint32_t op, uint32_t param)
{
    uint32_t res;

    __asm(
        "MOV r0, %[aop]\n"
        "MOV r1, %[aparam]\n" 
        "BKPT #0xAB\n"
        "MOV %[out], r0"
        : [out] "=r" (res)
        : [aop] "r" (op), [aparam] "r" (param)
    );

    return res;
}

uint32_t semihosting_clock()
{
    return semihost_call(SYS_CLOCK, 0);
}

uint64_t semihosting_elapsed()
{
    union
    {
        uint32_t arr[2];
        uint64_t val;
    } u;
    semihost_call(SYS_ELAPSED, (uint32_t)(&u.arr[0]));

    return u.val;
}

uint32_t semihosting_tickfreq()
{
    return semihost_call(SYS_TICKFREQ, 0);
}

_Noreturn void semihosting_exit()
{
    struct
    {
        uint32_t type;
        uint32_t subcode;
    } data;
    
    data.type = 0x20024; // ADP_Stopped_InternalError
    data.subcode = 1;

    semihost_call(SYS_EXIT, (uint32_t)&data);

    while(1) {} // This should not be reached
}
