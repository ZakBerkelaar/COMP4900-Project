#ifndef SEMIHOSTING_H
#define SEMIHOSTING_H

#include <stdint.h>

uint32_t semihosting_clock();
uint64_t semihosting_elapsed();
uint32_t semihosting_tickfreq();
_Noreturn void semihosting_exit();

#endif