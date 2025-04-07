#ifndef DEFS_H
#define DEFS_H

#define USE_SMP

extern void app_abort(void);

typedef uint32_t Time_t;

Time_t get_current_time(void);
Time_t get_time_frequency_ms(void);

#endif