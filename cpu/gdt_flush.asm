;=============================================================================
; WintakOS - gdt_flush.asm
; GDT'yi CPU'ya yükler ve segment register'ları günceller
;
; LGDT komutu GDT'yi yükler ama segment register'ları otomatik
; güncellemez. Far jump (jmp 0x08:.flush) ile CS güncellenir,
; MOV ile diğer segment register'ları güncellenir.
;=============================================================================

[bits 32]
[global gdt_flush]

gdt_flush:
    ;--- Parametre: GDT pointer yapısının adresi (stack'te) ---
    mov eax, [esp + 4]
    lgdt [eax]

    ;--- Veri segment register'larını Kernel Data (0x10) olarak ayarla ---
    mov ax, 0x10            ; GDT index 2 = Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;--- CS'yi güncellemek için far jump gereklidir ---
    ; CS doğrudan MOV ile değiştirilemez.
    ; Far jump: segment:offset formatında atlar.
    ; 0x08 = GDT index 1 = Kernel Code Segment
    jmp 0x08:.flush_done

.flush_done:
    ret
