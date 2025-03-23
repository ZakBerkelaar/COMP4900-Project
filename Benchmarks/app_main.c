#include <stdio.h>

void app_main(void)
{
#ifdef SCHED_LLREF
    printf("We are running llref\n");
#else
    printf("We are running edf\n");
#endif
}