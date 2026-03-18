/*============================================================================
 * WintakOS - isr.c
 * Interrupt Service Routines - Uygulama
 *
 * Assembly stub'ları tarafından çağrılan ana C handler.
 * Exception'ları yakalar, IRQ'ları dispatch eder, EOI gönderir.
 *==========================================================================*/

#include "isr.h"
#include "pic.h"
#include "vga.h"
#include "ports.h"

/*--- Handler tablosu (256 slot) ---*/
static isr_handler_t interrupt_handlers[IDT_ENTRIES] = { 0 };

/*--- CPU Exception isimleri (debug ve panic ekranı için) ---*/
static const char* const exception_names[32] = {
    "Division By Zero",             /*  0 */
    "Debug",                        /*  1 */
    "Non-Maskable Interrupt",       /*  2 */
    "Breakpoint",                   /*  3 */
    "Overflow",                     /*  4 */
    "Bound Range Exceeded",         /*  5 */
    "Invalid Opcode",               /*  6 */
    "Device Not Available",         /*  7 */
    "Double Fault",                 /*  8 */
    "Coprocessor Segment Overrun",  /*  9 */
    "Invalid TSS",                  /* 10 */
    "Segment Not Present",          /* 11 */
    "Stack-Segment Fault",          /* 12 */
    "General Protection Fault",     /* 13 */
    "Page Fault",                   /* 14 */
    "Reserved",                     /* 15 */
    "x87 Floating-Point Exception", /* 16 */
    "Alignment Check",              /* 17 */
    "Machine Check",                /* 18 */
    "SIMD Floating-Point",          /* 19 */
    "Virtualization Exception",     /* 20 */
    "Control Protection Exception", /* 21 */
    "Reserved", "Reserved",         /* 22-23 */
    "Reserved", "Reserved",         /* 24-25 */
    "Reserved", "Reserved",         /* 26-27 */
    "Hypervisor Injection",         /* 28 */
    "VMM Communication Exception",  /* 29 */
    "Security Exception",           /* 30 */
    "Reserved"                      /* 31 */
};

/*==========================================================================
 * Kernel Panic Ekranı
 *
 * İşlenemeyen bir CPU exception oluştuğunda kırmızı ekranda
 * detaylı bilgi gösterir ve sistemi durdurur.
 *========================================================================*/
static void kernel_panic(registers_t* regs)
{
    /* Kırmızı arka plan, beyaz yazı */
    vga_set_color(VGA_WHITE, VGA_RED);
    vga_clear();

    vga_puts("\n");
    vga_puts("  ============================================================\n");
    vga_puts("  KERNEL PANIC - Islenemeyen CPU Exception!\n");
    vga_puts("  ============================================================\n\n");

    /* Exception bilgisi */
    vga_puts("  Exception: ");
    if (regs->int_no < 32) {
        vga_puts(exception_names[regs->int_no]);
    } else {
        vga_puts("Bilinmeyen");
    }
    vga_puts(" (INT ");
    vga_put_dec(regs->int_no);
    vga_puts(")\n");

    vga_puts("  Error Code: ");
    vga_put_hex(regs->err_code);
    vga_puts("\n\n");

    /* Register dump */
    vga_puts("  --- Register Dump ---\n");

    vga_puts("  EAX="); vga_put_hex(regs->eax);
    vga_puts("  EBX="); vga_put_hex(regs->ebx);
    vga_puts("  ECX="); vga_put_hex(regs->ecx);
    vga_puts("\n");

    vga_puts("  EDX="); vga_put_hex(regs->edx);
    vga_puts("  ESI="); vga_put_hex(regs->esi);
    vga_puts("  EDI="); vga_put_hex(regs->edi);
    vga_puts("\n");

    vga_puts("  EBP="); vga_put_hex(regs->ebp);
    vga_puts("  ESP="); vga_put_hex(regs->esp);
    vga_puts("\n");

    vga_puts("  EIP="); vga_put_hex(regs->eip);
    vga_puts("  CS=");  vga_put_hex(regs->cs);
    vga_puts("  EFLAGS="); vga_put_hex(regs->eflags);
    vga_puts("\n\n");

    vga_puts("  Sistem durduruluyor.\n");
    vga_puts("  ============================================================\n");

    /* Sistemi durdur */
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

/*==========================================================================
 * Handler kayıt fonksiyonları
 *========================================================================*/
void isr_register_handler(uint8_t num, isr_handler_t handler)
{
    interrupt_handlers[num] = handler;
}

void isr_unregister_handler(uint8_t num)
{
    interrupt_handlers[num] = NULL;
}

/*==========================================================================
 * Ana interrupt dispatch fonksiyonu
 *
 * isr_stub.asm'deki isr_common_stub tarafından çağrılır.
 * Bu fonksiyon C calling convention ile çağrılır:
 *   void isr_handler(registers_t* regs)
 *
 * İki ana görev:
 *   1. Kayıtlı handler varsa çağır, yoksa exception ise panic
 *   2. IRQ ise PIC'e EOI gönder
 *========================================================================*/
void isr_handler(registers_t* regs)
{
    uint32_t int_no = regs->int_no;

    if (interrupt_handlers[int_no] != NULL) {
        /* Kayıtlı handler'ı çağır */
        interrupt_handlers[int_no](regs);
    } else if (int_no < 32) {
        /* İşlenemeyen CPU exception → kernel panic */
        kernel_panic(regs);
    }
    /* IRQ handler kaydedilmemişse sessizce geç (spurious interrupt) */

    /* Donanım IRQ'ları için End of Interrupt sinyali gönder */
    if (int_no >= 32 && int_no < 48) {
        pic_send_eoi((uint8_t)(int_no - 32));
    }
}
