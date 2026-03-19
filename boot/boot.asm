;=============================================================================
; WintakOS - boot.asm (Milestone 4)
; Multiboot2 + Framebuffer Istegi
;=============================================================================

MULTIBOOT2_MAGIC        equ 0xE85250D6
MULTIBOOT2_ARCHITECTURE equ 0
MULTIBOOT2_HEADER_LEN   equ (multiboot_header_end - multiboot_header_start)
MULTIBOOT2_CHECKSUM     equ 0x100000000 - (MULTIBOOT2_MAGIC + MULTIBOOT2_ARCHITECTURE + MULTIBOOT2_HEADER_LEN)

section .multiboot2
align 8

multiboot_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCHITECTURE
    dd MULTIBOOT2_HEADER_LEN
    dd MULTIBOOT2_CHECKSUM

    ;--- Framebuffer Tag ---
    ; type=5, flags=0, size=20
    ; width=800, height=600, depth=32
    align 8
    dw 5                            ; type: framebuffer request
    dw 0                            ; flags: not optional
    dd 20                           ; size of this tag
    dd 800                          ; width
    dd 600                          ; height
    dd 32                           ; bits per pixel

    ;--- End Tag ---
    align 8
    dw 0
    dw 0
    dd 8
multiboot_header_end:

section .bss
align 16

stack_bottom:
    resb 32768
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    push ebx                        ; Multiboot info pointer
    push eax                        ; Multiboot magic
    call kernel_main
    cli
.halt_loop:
    hlt
    jmp .halt_loop
