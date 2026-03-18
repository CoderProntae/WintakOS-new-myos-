;=============================================================================
; WintakOS - boot.asm
; Multiboot2 Uyumlu Kernel Giriş Noktası
;
; Bu dosya GRUB tarafından yüklenen ilk koddur.
; Görevleri:
;   1. Multiboot2 başlığını sağlamak (GRUB bunu arar)
;   2. Kernel stack'ini kurmak
;   3. GRUB'dan gelen bilgileri C kernel'ine aktarmak
;   4. kernel_main() fonksiyonunu çağırmak
;
; GRUB bizi 32-bit Protected Mode'da başlatır:
;   - A20 hattı açık
;   - Paging devre dışı
;   - Interrupt'lar devre dışı
;   - EAX = Multiboot2 magic (0x36D76289)
;   - EBX = Multiboot2 information structure adresi
;=============================================================================

;-----------------------------------------------------------------------------
; Multiboot2 Sabitleri
;
; Multiboot2 spesifikasyonu (GRUB2):
;   - Magic: 0xE85250D6
;   - Architecture: 0 = 32-bit protected mode i386
;   - Header length: başlığın toplam byte uzunluğu
;   - Checksum: magic + arch + length + checksum = 0 (mod 2^32)
;-----------------------------------------------------------------------------
MULTIBOOT2_MAGIC        equ 0xE85250D6
MULTIBOOT2_ARCHITECTURE equ 0
MULTIBOOT2_HEADER_LEN   equ (multiboot_header_end - multiboot_header_start)
MULTIBOOT2_CHECKSUM     equ 0x100000000 - (MULTIBOOT2_MAGIC + MULTIBOOT2_ARCHITECTURE + MULTIBOOT2_HEADER_LEN)

;-----------------------------------------------------------------------------
; Multiboot2 Başlık Bölümü
;
; Bu bölüm linker script'te ilk sıraya yerleştirilir.
; GRUB, ELF dosyasının ilk 32KB'ı içinde bu başlığı arar.
; 8-byte hizalama zorunludur (Multiboot2 spec).
;-----------------------------------------------------------------------------
section .multiboot2
align 8

multiboot_header_start:
    dd MULTIBOOT2_MAGIC             ; Magic number
    dd MULTIBOOT2_ARCHITECTURE      ; Mimari: i386 (32-bit)
    dd MULTIBOOT2_HEADER_LEN        ; Başlık uzunluğu (byte)
    dd MULTIBOOT2_CHECKSUM          ; Doğrulama toplamı

    ; === Bitiş Etiketi (End Tag) ===
    ; Her Multiboot2 başlığı bir end tag ile sonlanmalıdır.
    ; type = 0, flags = 0, size = 8
    dw 0                            ; type:  end tag
    dw 0                            ; flags: none
    dd 8                            ; size:  8 byte
multiboot_header_end:

;-----------------------------------------------------------------------------
; BSS Bölümü - Kernel Stack
;
; Neden 32KB? Kernel boot aşamasında derin fonksiyon çağrıları ve
; yerel değişkenler için yeterli alan sağlar. İleride interrupt
; handler'ları da bu stack'i kullanacak.
;
; Stack x86'da yukarıdan aşağıya (yüksek adresten düşüğe) büyür.
; Bu yüzden ESP = stack_top olarak ayarlanır.
;
; align 16: System V ABI uyumluluğu için 16-byte hizalama.
;-----------------------------------------------------------------------------
section .bss
align 16

stack_bottom:
    resb 32768                      ; 32 KB kernel stack alanı
stack_top:

;-----------------------------------------------------------------------------
; Text Bölümü - Giriş Noktası
;
; _start: Linker script'teki ENTRY(_start) ile eşleşir.
; kernel_main: C tarafında tanımlı ana kernel fonksiyonu (extern).
;-----------------------------------------------------------------------------
section .text
global _start
extern kernel_main

_start:
    ;=========================================================================
    ; 1. Stack Pointer'ı Ayarla
    ;=========================================================================
    mov esp, stack_top

    ;=========================================================================
    ; 2. GRUB'dan Gelen Bilgileri Kaydet
    ;
    ; GRUB, kernel'i yüklediğinde:
    ;   EAX = 0x36D76289 (Multiboot2 doğrulama değeri)
    ;   EBX = Multiboot2 information structure'ın bellek adresi
    ;
    ; C çağrı kuralı (cdecl): parametreler sağdan sola push edilir.
    ;   kernel_main(uint32_t magic, void* mbi_ptr)
    ;   → önce mbi_ptr (ebx), sonra magic (eax) push edilir
    ;=========================================================================
    push ebx                        ; Parametre 2: Multiboot info pointer
    push eax                        ; Parametre 1: Multiboot magic number

    ;=========================================================================
    ; 3. C Kernel'ini Çağır
    ;=========================================================================
    call kernel_main

    ;=========================================================================
    ; 4. Sonsuz Bekleme Döngüsü
    ;
    ; kernel_main() asla dönmemeli. Ama güvenlik için:
    ;   - CLI: Interrupt'ları kapat (hiçbir kesme bizi uyandırmasın)
    ;   - HLT: İşlemciyi düşük güç moduna al
    ;   - JMP: NMI (Non-Maskable Interrupt) durumunda tekrar durdur
    ;=========================================================================
    cli

.halt_loop:
    hlt
    jmp .halt_loop
