
cmake_minimum_required(VERSION 4.0)

project(
    COMP4900-Project
    LANGUAGES C
)

# Use C11
set(CMAKE_C_STANDARD 11)

# User arm compiler
set(CMAKE_C_COMPILER "arm-none-eabi-gcc")

# Common library used to share compile options
add_library(common INTERFACE)
target_compile_options(common INTERFACE
    -mcpu=cortex-m3
    -fno-builtin-printf
    -g
)

target_link_options(common INTERFACE
    -specs=nosys.specs -specs=nano.specs
    -nostartfiles
    -T "${CMAKE_CURRENT_SOURCE_DIR}/m3.ld"
    -mcpu=cortex-m3
    -fno-builtin-printf
    -g
)

target_include_directories(common INTERFACE
    ./include
)

set(COMMON_SOURCES
    ./startup.c
    ./printf.c
    Benchmarks/main.c
    Benchmarks/app_main.c
    Benchmarks/benchmarks.c
    Benchmarks/semihosting.c
)

add_executable(edf "${COMMON_SOURCES}")
add_executable(llref "${COMMON_SOURCES}")

set(TARGETS
    edf
    llref
)

# Add specific sources/includes
foreach(T ${TARGETS})
string(TOUPPER ${T} UPPER)
    set(KERNEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-${UPPER}")
    set(PORT_DIR "${KERNEL_DIR}/portable/GCC/ARM_CM3")
    set(SPECIFIC_SOURCES
        "${KERNEL_DIR}/tasks.c"
        "${KERNEL_DIR}/list.c"
        "${KERNEL_DIR}/queue.c"
        "${KERNEL_DIR}/timers.c"
        "${KERNEL_DIR}/event_groups.c"

        "${PORT_DIR}/port.c"
    )

    set(SPECIFIC_HEADERS
        "${KERNEL_DIR}/include"
        "${PORT_DIR}"
    )

    target_sources(${T} PRIVATE "${SPECIFIC_SOURCES}")
    target_include_directories(${T} PRIVATE "${SPECIFIC_HEADERS}")

    target_compile_definitions(${T} PUBLIC "PLATFORM_QEMU" "SCHED_${UPPER}")
endforeach(T)

# Add common directives
target_link_libraries(edf PRIVATE common)
target_link_libraries(llref PRIVATE common)