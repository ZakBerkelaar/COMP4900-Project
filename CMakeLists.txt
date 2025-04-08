
cmake_minimum_required(VERSION 4.0)

set(PLATFORM "qemu" CACHE STRING "Which platform to build for")

if(${PLATFORM} STREQUAL "qemu")
    message("Targeting qemu")
elseif(${PLATFORM} STREQUAL "rpi")
    message("Targeting rp2040")
endif()

if(${PLATFORM} STREQUAL "rpi")
    include(pico-sdk/pico_sdk_init.cmake)

    set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-LLREF")
    include(FreeRTOS_Kernel_import.cmake)
endif()

project(
    COMP4900-Project
    LANGUAGES C CXX ASM
)

# Use C11
set(CMAKE_C_STANDARD 11)
if(${PLATFORM} STREQUAL "rpi")
    set(CMAKE_CXX_STANDARD 17)
endif()

if(${PLATFORM} STREQUAL "rpi")
    pico_sdk_init()
endif()

# User arm compiler for qemu
if(${PLATFORM} STREQUAL "qemu")
    set(CMAKE_C_COMPILER "arm-none-eabi-gcc")
endif()

# Common library used to share compile options
add_library(common INTERFACE)

if(${PLATFORM} STREQUAL "qemu")
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
endif()

target_include_directories(common INTERFACE
    ./include
)

set(COMMON_SOURCES
    Benchmarks/main.c
    Benchmarks/app_main.c
    Benchmarks/benchmarks.c
)

if(${PLATFORM} STREQUAL "qemu")
    list(APPEND COMMON_SOURCES "./startup.c" "./printf.c" "Benchmarks/semihosting.c")
endif()

add_executable(edf "${COMMON_SOURCES}")
add_executable(llref "${COMMON_SOURCES}")

set(TARGETS
    edf
    llref
)

# Add specific sources/includes
foreach(T ${TARGETS})
    string(TOUPPER ${PLATFORM} PLATFORM_UPPER)
    string(TOUPPER ${T} UPPER)
    if(${PLATFORM} STREQUAL "qemu")
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
    endif()

    target_compile_definitions(${T} PUBLIC "PLATFORM_${PLATFORM_UPPER}" "SCHED_${UPPER}")
endforeach(T)

# Add common directives
target_link_libraries(edf PRIVATE common)
target_link_libraries(llref PRIVATE common)

if(${PLATFORM} STREQUAL "rpi")
    target_link_libraries(edf PRIVATE pico_stdlib FreeRTOS-Kernel)
    target_link_libraries(llref PRIVATE pico_stdlib FreeRTOS-Kernel)

    pico_enable_stdio_usb(edf 1)
    pico_enable_stdio_uart(edf 0)
    pico_enable_stdio_usb(llref 1)
    pico_enable_stdio_uart(llref 0)
    pico_add_extra_outputs(edf)
    pico_add_extra_outputs(llref)
endif()