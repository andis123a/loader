TOOLCHAIN_PREFIX ?=
CC = $(TOOLCHAIN_PREFIX)gcc
AR = $(TOOLCHAIN_PREFIX)ar
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy

FW_DEFINE ?=
EXTRA_CFLAGS ?=
BUILD_DIR ?= .

ifeq ($(strip $(FW_DEFINE)),)
$(error FW_DEFINE must be set, e.g. FW_DEFINE=PS4_7_00)
endif

CFLAGS=$(CFLAG) -D$(FW_DEFINE) $(EXTRA_CFLAGS) -DKASLR -DNO_SYMTAB -DDO_NOT_REMAP_RWX
CFLAGS += -march=btver2 -masm=intel -std=gnu11 -ffreestanding -fno-common \
	-fPIE -pie -fno-stack-protector -fomit-frame-pointer -nostdlib -nostdinc \
	-fno-asynchronous-unwind-tables \
	-Os -Wall -Werror -Wl,--no-dynamic-linker,--build-id=none,-T,kexec.ld,--nmagic \
	-mcmodel=small -mno-red-zone

SOURCES := kernel.c kexec.c linux_boot.c linux_thunk.S uart.c firmware.c \
	acpi.c crc32.c

OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SOURCES))))

all: $(BUILD_DIR)/libkexec.a $(BUILD_DIR)/kexec.bin

$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/%.o: %.c *.h | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: %.S | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/libkexec.a: $(OBJS)
	$(AR) -rc $@ $(OBJS)

$(BUILD_DIR)/kexec.elf: $(BUILD_DIR)/libkexec.a kexec.ld
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f $(BUILD_DIR)/libkexec.a $(BUILD_DIR)/kexec.elf $(BUILD_DIR)/kexec.bin $(OBJS)
