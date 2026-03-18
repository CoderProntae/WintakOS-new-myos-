#=============================================================================
# WintakOS - Makefile
# Hem lokal hem GitHub Actions tarafindan kullanilir
#=============================================================================

#--- Arac Zinciri ---
ifeq ($(shell which i686-elf-gcc 2>/dev/null),)
    CC           := gcc
    CFLAGS_ARCH  := -m32
    LDFLAGS_ARCH := -m elf_i386
else
    CC           := i686-elf-gcc
    CFLAGS_ARCH  :=
    LDFLAGS_ARCH :=
endif

AS   := nasm
QEMU := qemu-system-i386

#--- Bayraklar ---
ASFLAGS := -f elf32

CFLAGS  := $(CFLAGS_ARCH) -std=gnu99 -ffreestanding -O2 \
           -Wall -Wextra -Werror \
           -fno-stack-protector -fno-pie -fno-pic \
           -Iinclude -Ikernel

#--- Dosyalar ---
ASM_SOURCES := boot/boot.asm
C_SOURCES   := kernel/kernel.c kernel/vga.c

ASM_OBJECTS := $(ASM_SOURCES:.asm=.o)
C_OBJECTS   := $(C_SOURCES:.c=.o)
ALL_OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

KERNEL_BIN  := wintakos.bin
ISO_FILE    := wintakos.iso
ISO_DIR     := iso

#=============================================================================
.PHONY: all clean run debug

all: $(ISO_FILE)
	@echo ""
	@echo "========================================"
	@echo "  WintakOS basariyla derlendi!"
	@echo "  ISO: $(ISO_FILE)"
	@echo "========================================"

boot/boot.o: boot/boot.asm
	@echo "  [ASM]  $<"
	@$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	@echo "  [CC]   $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(ALL_OBJECTS)
	@echo "  [LINK] $@"
	@$(CC) $(CFLAGS_ARCH) -T linker.ld -o $@ -ffreestanding -O2 -nostdlib \
		$(ALL_OBJECTS) -lgcc

$(ISO_FILE): $(KERNEL_BIN)
	@echo "  [ISO]  $@"
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/
	@cp boot/grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $(ISO_FILE) $(ISO_DIR) 2>/dev/null

run: $(ISO_FILE)
	@$(QEMU) -cdrom $(ISO_FILE) -m 128M

debug: $(ISO_FILE)
	@$(QEMU) -cdrom $(ISO_FILE) -m 128M -s -S

clean:
	@rm -f $(ALL_OBJECTS) $(KERNEL_BIN) $(ISO_FILE)
	@rm -rf $(ISO_DIR)
	@echo "  Temizlendi."
