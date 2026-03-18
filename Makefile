#=============================================================================
# WintakOS - Makefile (Phase 1)
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
           -Iinclude -Ikernel -Icpu

LINK_FLAGS := $(CFLAGS_ARCH) -T linker.ld -ffreestanding -O2 -nostdlib \
              -no-pie -static \
              -Wl,--build-id=none \
              -Wl,-z,max-page-size=0x1000

#--- Kaynak Dosyalari ---
ASM_SOURCES := boot/boot.asm 
