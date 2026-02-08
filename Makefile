# BasicOS Makefile
# Builds a hobby operating system from scratch

# Architecture and toolchain
ARCH := x86_64
CC := gcc
AS := nasm
LD := ld

# Directories
KERNEL_DIR := kernel
DRIVERS_DIR := drivers
GUI_DIR := gui
APPS_DIR := apps
LIB_DIR := lib
BUILD_DIR := build
BOOTLOADER_DIR := bootloader

# Compiler flags
CFLAGS := -Wall -Wextra -Werror -std=c11 -ffreestanding -fno-stack-protector \
          -fno-pic -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=kernel -I$(KERNEL_DIR)/include -I$(DRIVERS_DIR)/include \
          -I$(GUI_DIR)/include -I$(LIB_DIR)/include -O2

# Linker flags
LDFLAGS := -nostdlib -static -z max-page-size=0x1000 -T linker.ld

# Assembly flags
ASFLAGS := -f elf64

# Source files
KERNEL_C_SRC := $(shell find $(KERNEL_DIR) -name '*.c')
KERNEL_ASM_SRC := $(shell find $(KERNEL_DIR) -name '*.s' -o -name '*.asm')
DRIVERS_C_SRC := $(shell find $(DRIVERS_DIR) -name '*.c' 2>/dev/null || true)
GUI_C_SRC := $(shell find $(GUI_DIR) -name '*.c' 2>/dev/null || true)
APPS_C_SRC := $(shell find $(APPS_DIR) -name '*.c' 2>/dev/null || true)
LIB_C_SRC := $(shell find $(LIB_DIR) -name '*.c' 2>/dev/null || true)

# Object files
KERNEL_C_OBJ := $(KERNEL_C_SRC:.c=.o)
KERNEL_ASM_OBJ := $(KERNEL_ASM_SRC:.asm=.o)
KERNEL_ASM_OBJ := $(KERNEL_ASM_OBJ:.s=.o)
DRIVERS_C_OBJ := $(DRIVERS_C_SRC:.c=.o)
GUI_C_OBJ := $(GUI_C_SRC:.c=.o)
APPS_C_OBJ := $(APPS_C_SRC:.c=.o)
LIB_C_OBJ := $(LIB_C_SRC:.c=.o)

ALL_OBJ := $(KERNEL_C_OBJ) $(KERNEL_ASM_OBJ) $(DRIVERS_C_OBJ) $(GUI_C_OBJ) \
           $(APPS_C_OBJ) $(LIB_C_OBJ)

# Output
KERNEL_BIN := $(BUILD_DIR)/kernel.elf
ISO_FILE := $(BUILD_DIR)/basicOS.iso

# Limine files
LIMINE_DIR := limine
LIMINE_BIN := $(LIMINE_DIR)/limine-deploy
LIMINE_CFG := $(BOOTLOADER_DIR)/limine.cfg

.PHONY: all clean iso run run-uefi download-limine

all: $(ISO_FILE)

# Download and build Limine bootloader
download-limine:
	@if [ ! -d "$(LIMINE_DIR)" ]; then \
		echo "Downloading Limine bootloader..."; \
		git clone https://github.com/limine-bootloader/limine.git --branch=v5.x-branch-binary --depth=1; \
		$(MAKE) -C $(LIMINE_DIR); \
	fi

# Compile C source files
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
%.o: %.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(KERNEL_BIN): $(ALL_OBJ) linker.ld
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJ)
	@echo "Kernel built: $(KERNEL_BIN)"

# Create ISO image
$(ISO_FILE): $(KERNEL_BIN) download-limine $(LIMINE_CFG)
	@mkdir -p $(BUILD_DIR)/iso_root
	@cp $(KERNEL_BIN) $(BUILD_DIR)/iso_root/
	@mkdir -p $(BUILD_DIR)/iso_root/boot
	@cp $(LIMINE_CFG) $(BUILD_DIR)/iso_root/boot/
	@cp $(LIMINE_DIR)/limine-bios.sys $(BUILD_DIR)/iso_root/boot/limine.sys
	@cp $(LIMINE_DIR)/limine-bios-cd.bin $(BUILD_DIR)/iso_root/boot/limine-cd.bin
	@cp $(LIMINE_DIR)/limine-uefi-cd.bin $(BUILD_DIR)/iso_root/boot/limine-cd-efi.bin
	@mkdir -p $(BUILD_DIR)/iso_root/EFI/BOOT
	@cp $(LIMINE_DIR)/BOOTX64.EFI $(BUILD_DIR)/iso_root/EFI/BOOT/
	@xorriso -as mkisofs -b boot/limine-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine-cd-efi.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		$(BUILD_DIR)/iso_root -o $(ISO_FILE) 2>/dev/null
	@$(LIMINE_DIR)/limine bios-install $(ISO_FILE) 2>/dev/null
	@echo "ISO created: $(ISO_FILE)"

# Run in QEMU (BIOS mode)
run: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -serial stdio \
		-vga std -display gtk

# Run in QEMU (UEFI mode)
run-uefi: $(ISO_FILE)
	qemu-system-x86_64 -cdrom $(ISO_FILE) -m 256M -serial stdio \
		-bios /usr/share/ovmf/OVMF.fd -vga std -display gtk

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	find . -name '*.o' -delete
	@echo "Cleaned build artifacts"

# Clean everything including Limine
distclean: clean
	rm -rf $(LIMINE_DIR)
	@echo "Cleaned everything"
