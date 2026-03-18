#include "isr.h"
#include "idt.h"
#include "gdt.h"
#include "pic.h"
#include "../kernel/vga.h"

static irq_handler_t irq_handlers[16];

static const char* exception_names[32] = {
    "Division By Zero",           "Debug",
    "Non Maskable Interrupt",     "Breakpoint",
    "Overflow",                   "Bound Range Exceeded",
    "Invalid Opcode",             "Device Not Available",
    "Double Fault",               "Coprocessor Overrun",
    "Invalid TSS",                "Segment Not Present",
    "Stack-Segment Fault",        "General Protection Fault",
    "Page Fault",                 "Reserved",
    "x87 FPU Error",              "Alignment Check",
    "Machine Check",              "SIMD FPU Exception",
    "Virtualization Exception",   "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved",
    "Reserved", "Reserved",       "Hypervisor Injection",
    "VMM Communication",          "Security Exception",
    "Reserved"
};

void irq_register_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void isr_handler(registers_t* regs)
{
    if (regs->int_no < 32) {
        vga_set_color(VGA_WHITE, VGA_RED);
        vga_puts("\n EXCEPTION: ");
        vga_puts(exception_names[regs->int_no]);
        vga_puts(" (#");
        vga_put_dec(regs->int_no);
        vga_puts(")\n");

        vga_set_color(VGA_WHITE, VGA_BLACK);
        vga_puts("  Error Code: "); vga_put_hex(regs->err_code);
        vga_puts("\n  EIP:        "); vga_put_hex(regs->eip);
        vga_puts("\n  CS:         "); vga_put_hex(regs->cs);
        vga_puts("\n  EFLAGS:     "); vga_put_hex(regs->eflags);
        vga_puts("\n\n  Sistem durduruluyor.\n");

        while (1) { __asm__ volatile("cli; hlt"); }
    }
    else if (regs->int_no >= 32 && regs->int_no <= 47) {
        uint8_t irq = (uint8_t)(regs->int_no - 32);

        if (irq_handlers[irq]) {
            irq_handlers[irq](regs);
        }

        pic_send_eoi(irq);
    }
}

void isr_init(void)
{
    idt_set_gate(0,  (uint32_t)isr0,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, GDT_KERNEL_CODE, 0x8E);

    idt_set_gate(32, (uint32_t)irq0,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, GDT_KERNEL_CODE, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, GDT_KERNEL_CODE, 0x8E);
}
