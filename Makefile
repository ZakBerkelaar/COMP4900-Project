CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
SIZE = arm-none-eabi-size
MAKE = make

KERNEL_DIR = ./FreeRTOS-$(PROJECT)
PORT_DIR = $(KERNEL_DIR)/portable/GCC/ARM_CM3
OUT_DIR = ./out/$(PROJECT)
IMAGE = $(OUT_DIR)/$(PROJECT).out

CFLAGS += -mcpu=cortex-m3
CFLAGS += $(INCLUDE_DIRS)
CFLAGS += -fno-builtin-printf
CFLAGS += -DSCHED_$(PROJECT)
CFLAGS += -DPLATFORM_QEMU
CFLAGS += -g

LDFLAGS += -specs=nosys.specs -specs=nano.specs
LDFLAGS += -nostartfiles
LDFLAGS += -T m3.ld

INCLUDE_DIRS += -I$(KERNEL_DIR)/include
INCLUDE_DIRS += -I$(PORT_DIR)
INCLUDE_DIRS += -I./include

KERNEL_SOURCE_FILES += $(KERNEL_DIR)/tasks.c
KERNEL_SOURCE_FILES += $(KERNEL_DIR)/list.c
KERNEL_SOURCE_FILES += $(KERNEL_DIR)/queue.c
KERNEL_SOURCE_FILES += $(KERNEL_DIR)/timers.c
KERNEL_SOURCE_FILES += $(KERNEL_DIR)/event_groups.c
#KERNEL_SOURCE_FILES += $(KERNEL_DIR)/portable/MemMang/heap_4.c
KERNEL_SOURCE_FILES += $(PORT_DIR)/port.c

COMMON_SOURCE_FILES += ./startup.c
COMMON_SOURCE_FILES += ./printf.c
COMMON_SOURCE_FILES += Benchmarks/main.c
COMMON_SOURCE_FILES += Benchmarks/app_main.c
COMMON_SOURCE_FILES += Benchmarks/benchmarks.c
COMMON_SOURCE_FILES += Benchmarks/semihosting.c

KERNEL_OBJS_NO_PATH = $(KERNEL_SOURCE_FILES:%.c=%.o)
KERNEL_OBJS_TEST = $(KERNEL_OBJS_NO_PATH:$(KERNEL_DIR)%=.%)
KERNEL_OBJS_OUT = $(KERNEL_OBJS_TEST:%.o=$(OUT_DIR)/%.o)

COMMON_OBJS = $(COMMON_SOURCE_FILES:%.c=%.o)
COMMON_OBJS_OUT = $(COMMON_OBJS:%.o=$(OUT_DIR)/%.o)

.PHONY: clean

all: edf llref

edf: PROJECT = EDF
edf:
	$(MAKE) $(IMAGE) PROJECT=$(PROJECT)

llref: PROJECT = LLREF
llref:
	$(MAKE) $(IMAGE) PROJECT=$(PROJECT)

clean:
	rm -rf out/

$(OUT_DIR):
	mkdir -p $@

$(OUT_DIR)/%.o: $(KERNEL_DIR)/%.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(IMAGE): $(KERNEL_OBJS_OUT) $(COMMON_OBJS_OUT) Makefile
	$(LD) $(CFLAGS) $(LDFLAGS) $(KERNEL_OBJS_OUT) $(COMMON_OBJS_OUT) -o $(IMAGE)
	$(SIZE) $(IMAGE)