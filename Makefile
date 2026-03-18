#=============================================================================
# WintakOS - Makefile
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

#--- DÜZELTME: Link bayraklari ayri tanimlandi ---
# -no-pie            : PIE devre disi (relocation uretme)
# -static            : Dinamik baglama yapma
# -Wl,--build-id=none: .note.gnu.build-id bolumunu uretme
# -Wl,-z,max-page-size=0x1000 : Sayfa boyutunu 4KB'a sabitle
LINK_FLAGS := $(CFLAGS_ARCH) -T linker.ld -ffreestanding -O2 -nostdlib \
              -no-pie -static \
              -Wl,--build-id=none \
              -Wl,-z,max-page-size=0x1000

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
.PHONY: all clean run debug verify

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
	@$(CC) $(LINK_FLAGS) -o $@ $(ALL_OBJECTS) -lgcc
	@echo "  [CHECK] Multiboot2 ve ELF dogrulama..."
	@file $@ | grep "32-bit" > /dev/null && echo "  [OK] 32-bit ELF" || echo "  [WARN] 32-bit degil!"
	@readelf -r $@ 2>/dev/null | grep -q "reloc" && \
		echo "  [WARN] Relocation kayitlari mevcut!" || \
		echo "  [OK] Relocation yok"

$(ISO_FILE): $(KERNEL_BIN)
	@echo "  [ISO]  $@"
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(KERNEL_BIN) $(ISO_DIR)/boot/
	@cp boot/grub.cfg $(ISO_DIR)/boot/grub/
	@grub-mkrescue -o $(ISO_FILE) $(ISO_DIR) 2>/dev/null

#--- Dogrulama (CI icin faydali) ---
verify: $(KERNEL_BIN)
	@echo "=== ELF Bilgisi ==="
	@file $(KERNEL_BIN)
	@echo ""
	@echo "=== ELF Basligi ==="
	@readelf -h $(KERNEL_BIN) | grep -E "Class|Type|Entry"
	@echo ""
	@echo "=== Relocation Kontrolu ==="
	@readelf -r $(KERNEL_BIN) 2>/dev/null || echo "Relocation yok - TEMIZ"
	@echo ""
	@echo "=== Multiboot2 Kontrolu ==="
	@grub-file --is-x86-multiboot2 $(KERNEL_BIN) && \
		echo "BASARILI: Multiboot2 uyumlu" || \
		echo "BASARISIZ: Multiboot2 uyumsuz"

run: $(ISO_FILE)
	@$(QEMU) -cdrom $(ISO_FILE) -m 128M

debug: $(ISO_FILE)
	@$(QEMU) -cdrom $(ISO_FILE) -m 128M -s -S

clean:
	@rm -f $(ALL_OBJECTS) $(KERNEL_BIN) $(ISO_FILE)
	@rm -rf $(ISO_DIR)
	@echo "  Temizlendi."
