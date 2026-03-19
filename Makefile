ifeq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC           := gcc
    CFLAGS_ARCH  := -m32
else
    CC           := i686-elf-gcc
    CFLAGS_ARCH  :=
endif

AS   := nasm
QEMU := qemu-system-i386

ASFLAGS := -f elf32

CFLAGS  := $(CFLAGS_ARCH) -std=gnu99 -ffreestanding -O2 \
           -Wall -Wextra -Werror \
           -fno-stack-protector -fno-pie -fno-pic \
           -I. -Iinclude -Ikernel -Icpu -Idrivers -Imemory -Ilib -Igui -Iapps -Ifs

LINK_FLAGS := $(CFLAGS_ARCH) -T linker.ld -ffreestanding -O2 -nostdlib \
              -no-pie -static \
              -Wl,--build-id=none \
              -Wl,-z,max-page-size=0x1000

ASM_SOURCES := boot/boot.asm \
               cpu/gdt_flush.asm \
               cpu/idt_flush.asm \
               cpu/isr_stub.asm

C_SOURCES   := kernel/kernel.c \
               kernel/vga.c \
               cpu/gdt.c \
               cpu/idt.c \
               cpu/isr.c \
               cpu/pic.c \
               cpu/pit.c \
               cpu/rtc.c \
               drivers/keyboard.c \
               drivers/vga_font.c \
               drivers/framebuffer.c \
               drivers/fbconsole.c \
               drivers/mouse.c \
               memory/pmm.c \
               memory/heap.c \
               lib/string.c \
               gui/desktop.c \
               gui/window.c \
               gui/widget.c \
               apps/terminal.c \
               apps/setup.c \
               fs/ramfs.c

ASM_OBJECTS := $(ASM_SOURCES:.asm=.o)
C_OBJECTS   := $(C_SOURCES:.c=.o)
ALL_OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

KERNEL_BIN := wintakos.bin
ISO_FILE   := wintakos.iso
ISO_DIR    := iso

.PHONY: all clean run debug verify

all: $(ISO_FILE)
	@echo "  WintakOS Milestone 8 derlendi! ISO: $(ISO_FILE)"

boot/boot.o: boot/boot.asm
	@$(AS) $(ASFLAGS) $< -o $@

cpu/%.o: cpu/%.asm
	@$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

cpu/%.o: cpu/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

drivers/%.o: drivers/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

memory/%.o: memory/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

lib/%.o: lib/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

gui/%.o: gui/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

apps/%.o: apps/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

fs/%.o: fs/%.c
	@$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(ALL_OBJECTS)
	@$(CC) $(LINK_FLAGS) -o $@ $(ALL_OBJECTS) -lgcc

$(ISO_FILE): $(KERNEL_BIN)
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/
	@cp boot/grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $(ISO_FILE) $(ISO_DIR) 2>/dev/null

verify: $(KERNEL_BIN)
	@file $(KERNEL_BIN)
	@grub-file --is-x86-multiboot2 $(KERNEL_BIN) && echo "Multiboot2 OK" || echo "FAIL"

run: $(ISO_FILE)
	@$(QEMU) -cdrom $(ISO_FILE) -m 512M

debug: $(ISO_FILE)
	@$(QEMU) -cdrom $(ISO_FILE) -m 512M -s -S

clean:
	@rm -f $(ALL_OBJECTS) $(KERNEL_BIN) $(ISO_FILE)
	@rm -rf $(ISO_DIR)
