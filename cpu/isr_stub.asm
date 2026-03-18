;=============================================================================
; WintakOS - isr_stub.asm
; Interrupt Service Routine Assembly Stub'ları
;
; Her interrupt için bir giriş noktası tanımlar.
; CPU bazı exception'larda otomatik error code push eder, bazılarında etmez.
; Tutarlılık için error code push etmeyenlere dummy 0 ekliyoruz.
;
; Akış:
;   1. ISR/IRQ macro → error code + int_no push → common stub'a atla
;   2. Common stub → register'ları kaydet → C handler çağır → geri yükle
;   3. IRET ile interrupt'tan dön
;=============================================================================

[bits 32]

;--- C tarafındaki handler fonksiyonu ---
[extern isr_handler]

;=============================================================================
; MACRO: ISR_NOERR - CPU'nun error code PUSH ETMEDİĞİ exception'lar
;
; Stack durumu (macro çalıştıktan sonra):
;   ... EIP, CS, EFLAGS (CPU tarafından)
;   0          ← dummy error code (bizim push'umuz)
;   int_no     ← interrupt numarası (bizim push'umuz)
;=============================================================================
%macro ISR_NOERR 1
[global isr%1]
isr%1:
    push dword 0            ; Dummy error code
    push dword %1           ; Interrupt numarası
    jmp isr_common_stub
%endmacro

;=============================================================================
; MACRO: ISR_ERR - CPU'nun error code PUSH ETTİĞİ exception'lar
;
; Stack durumu (macro çalıştıktan sonra):
;   ... EIP, CS, EFLAGS (CPU tarafından)
;   error_code ← CPU tarafından push edilen
;   int_no     ← interrupt numarası (bizim push'umuz)
;=============================================================================
%macro ISR_ERR 1
[global isr%1]
isr%1:
    ; Error code zaten CPU tarafından push edildi
    push dword %1           ; Interrupt numarası
    jmp isr_common_stub
%endmacro

;=============================================================================
; MACRO: IRQ - Donanım IRQ'ları
;
; IRQ numarasını ve karşılık gelen INT numarasını alır.
; Örnek: IRQ 0, 32 → irq0 fonksiyonu, INT 32 olarak işlenir
;=============================================================================
%macro IRQ 2
[global irq%1]
irq%1:
    push dword 0            ; Dummy error code
    push dword %2           ; Karşılık gelen INT numarası
    jmp isr_common_stub
%endmacro

;=============================================================================
; CPU Exception'ları (INT 0-31)
;
; Error code push edenler: 8, 10, 11, 12, 13, 14, 17, 21, 29, 30
; Geri kalanlar push etmez → dummy 0 eklenir
;=============================================================================
ISR_NOERR 0                ; #DE  Divide Error
ISR_NOERR 1                ; #DB  Debug Exception
ISR_NOERR 2                ; ---  NMI Interrupt
ISR_NOERR 3                ; #BP  Breakpoint
ISR_NOERR 4                ; #OF  Overflow
ISR_NOERR 5                ; #BR  Bound Range Exceeded
ISR_NOERR 6                ; #UD  Invalid Opcode
ISR_NOERR 7                ; #NM  Device Not Available
ISR_ERR   8                ; #DF  Double Fault
ISR_NOERR 9                ; ---  Coprocessor Segment Overrun
ISR_ERR   10               ; #TS  Invalid TSS
ISR_ERR   11               ; #NP  Segment Not Present
ISR_ERR   12               ; #SS  Stack-Segment Fault
ISR_ERR   13               ; #GP  General Protection Fault
ISR_ERR   14               ; #PF  Page Fault
ISR_NOERR 15               ; ---  Reserved
ISR_NOERR 16               ; #MF  x87 Floating-Point
ISR_ERR   17               ; #AC  Alignment Check
ISR_NOERR 18               ; #MC  Machine Check
ISR_NOERR 19               ; #XM  SIMD Floating-Point
ISR_NOERR 20               ; #VE  Virtualization Exception
ISR_ERR   21               ; #CP  Control Protection
ISR_NOERR 22               ; ---  Reserved
ISR_NOERR 23               ; ---  Reserved
ISR_NOERR 24               ; ---  Reserved
ISR_NOERR 25               ; ---  Reserved
ISR_NOERR 26               ; ---  Reserved
ISR_NOERR 27               ; ---  Reserved
ISR_NOERR 28               ; ---  Reserved
ISR_ERR   29               ; #VC  VMM Communication
ISR_ERR   30               ; #SX  Security Exception
ISR_NOERR 31               ; ---  Reserved

;=============================================================================
; Donanım IRQ'ları (IRQ 0-15 → INT 32-47)
;
; PIC tarafından remap edildikten sonraki interrupt numaraları:
;   Master PIC: IRQ 0-7  → INT 32-39
;   Slave PIC:  IRQ 8-15 → INT 40-47
;=============================================================================
IRQ  0, 32                 ; PIT Timer
IRQ  1, 33                 ; Klavye
IRQ  2, 34                 ; Cascade (slave PIC bağlantısı)
IRQ  3, 35                 ; COM2
IRQ  4, 36                 ; COM1
IRQ  5, 37                 ; LPT2
IRQ  6, 38                 ; Floppy Disk
IRQ  7, 39                 ; LPT1 / Spurious
IRQ  8, 40                 ; CMOS Real-Time Clock
IRQ  9, 41                 ; Free / ACPI
IRQ 10, 42                 ; Free
IRQ 11, 43                 ; Free
IRQ 12, 44                 ; PS/2 Mouse
IRQ 13, 45                 ; FPU / Coprocessor
IRQ 14, 46                 ; Primary ATA
IRQ 15, 47                 ; Secondary ATA

;=============================================================================
; Ortak Interrupt Stub
;
; Tüm ISR/IRQ macro'ları buraya atlar.
; Görevleri:
;   1. Tüm genel amaçlı register'ları kaydet (pusha)
;   2. Veri segment'i kaydet ve kernel data segment'e geç
;   3. registers_t yapısının adresini parametre olarak ilet
;   4. C handler'ı çağır
;   5. Segment ve register'ları geri yükle
;   6. Error code ve int_no'yu temizle
;   7. IRET ile interrupt'tan dön
;=============================================================================
isr_common_stub:
    ;--- 1. Genel amaçlı register'ları kaydet ---
    pusha                       ; EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    ;--- 2. Veri segment'i kaydet, kernel data segment'e geç ---
    mov ax, ds
    push eax                    ; DS değerini kaydet

    mov ax, 0x10                ; 0x10 = Kernel Data Segment (GDT index 2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;--- 3. C handler'ı çağır ---
    ; ESP şu an registers_t yapısının başını gösteriyor.
    ; Bu adresi parametre olarak push ediyoruz.
    push esp                    ; registers_t* regs parametre olarak
    call isr_handler            ; C handler: void isr_handler(registers_t*)
    add esp, 4                  ; Push edilen parametreyi temizle

    ;--- 4. Veri segment'i geri yükle ---
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;--- 5. Register'ları geri yükle ---
    popa                        ; EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX

    ;--- 6. Stack'teki error code ve int_no'yu temizle ---
    add esp, 8                  ; 2 × 4 byte (int_no + err_code)

    ;--- 7. Interrupt'tan dön ---
    iret
