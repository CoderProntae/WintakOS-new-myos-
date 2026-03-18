;=============================================================================
; WintakOS - idt_flush.asm
; LIDT komutu ile IDT'yi CPU'ya yükler
;=============================================================================

[bits 32]
[global idt_flush]

idt_flush:
    mov eax, [esp + 4]     ; Parametre: idt_ptr yapısının adresi
    lidt [eax]             ; IDT'yi yükle
    ret
