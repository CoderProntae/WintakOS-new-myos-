/*============================================================================
 * WintakOS - isr.h
 * Interrupt Service Routines - Başlık
 *
 * CPU interrupt'ı aldığında çağrılan handler sistemi.
 * Assembly stub register'ları kaydettikten sonra C handler'ı çağırır.
 * C handler, kayıtlı fonksiyonu dispatch eder (PIT, klavye, vb.).
 *==========================================================================*/

#ifndef WINTAKOS_ISR_H
#define WINTAKOS_ISR_H

#include "types.h"

/*==========================================================================
 * Register Yapısı
 *
 * isr_stub.asm'deki common stub tarafından stack üzerine inşa edilir.
 * Stack düzeni (düşük adresten yükseğe = struct başından sonuna):
 *
 *   ds         ← pusha'dan önce push edilen veri segment değeri
 *   edi..eax   ← pusha tarafından kaydedilen genel amaçlı register'lar
 *   int_no     ← ISR stub tarafından push edilen interrupt numarası
 *   err_code   ← CPU tarafından (veya stub'daki dummy 0) push edilen
 *   eip        ← CPU tarafından interrupt anında push edilen
 *   cs, eflags ← CPU tarafından push edilen
 *   useresp,ss ← Sadece privilege değişiminde (ring 3→0) push edilir
 *========================================================================*/
typedef struct {
    /* Veri segment seçici (common stub tarafından push edilir) */
    uint32_t ds;

    /* pusha tarafından kaydedilen register'lar */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;

    /* ISR stub tarafından push edilen */
    uint32_t int_no;
    uint32_t err_code;

    /* CPU tarafından otomatik push edilen */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t useresp;   /* Sadece privilege değişiminde geçerli */
    uint32_t ss;        /* Sadece privilege değişiminde geçerli */
} __attribute__((packed)) registers_t;

/*--- Interrupt handler fonksiyon tipi ---*/
typedef void (*isr_handler_t)(registers_t* regs);

/*--- Genel Fonksiyonlar ---*/

/* Belirli bir interrupt numarasına handler kaydet */
void isr_register_handler(uint8_t num, isr_handler_t handler);

/* Handler kaydını kaldır */
void isr_unregister_handler(uint8_t num);

#endif /* WINTAKOS_ISR_H */
