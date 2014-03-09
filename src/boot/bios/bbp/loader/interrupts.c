/*

Helper functions for operations interrupts
==========================================

This contains imports from ASM stub subroutines that catch all the neccessary
exception interrupts and calls a C function interrupt_handler().
It also contains LIDT wrapper function and C functions to handle interrupts.

License (BSD-3)
===============

Copyright (c) 2012, Gusts 'gusC' Kaksis <gusts.kaksis@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "../config.h"
#include "interrupts.h"
#include "pic.h"
#include "lib.h"
#include "io.h"
#include "paging.h"
#include "cr.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

/**
* Exception names
*/
static char *ints[] = {
  "Division by zero",
  "Debug exception",
  "NMI interrupt",
  "Breakpoint",
  "INTO overflow",
  "BOUND exception",
  "Invalid opcode",
  "No FPU",
  "Double Fault!",
  "FPU segment overrun",
  "Bad TSS",
  "Segment not present",
  "Stack fault",
  "GPF",
  "Page fault",
  "",
  "FPU Exception",
  "Alignament check exception",
  "Machine check exception"
};
/**
* Interrupt Descriptor Table
*/
idt_entry_t idt[256];
/**
* Interrupt Descriptor Table pointer
*/
idt_ptr_t idt_ptr;
/**
* Interrupt handlers
*/
isr_handler_t isr_handlers[256];
/**
* Interrupt handlers
*/
irq_handler_t irq_handlers[16];
/**
* Current interrupt status
*/
static bool _int_enabled = false;

/**
* Set IDT pointer
* @see interrupts.asm
* @param idt_ptr - an address of IDT pointer structure in memory
* @return void
*/
extern void idt_set(idt_ptr_t *idt_ptr);

// Defined in interrupts.asm (with macros!)
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
// Defined in interrupts.asm (with macros!)
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

static void idt_set_entry(uint8 num, uint64 addr, uint8 flags){
	idt[num].offset_lo = (uint16)(addr & 0xFFFF);
	idt[num].offset_hi = (uint16)((addr >> 16) & 0xFFFF);
	idt[num].offset_64 = (uint32)((addr >> 32) & 0xFFFFFFFFF);
    idt[num].slector = 0x08; // Allways a code selector
	idt[num].type = flags;
	idt[num].reserved1 = 0; // Zero out
    idt[num].reserved2 = 0; // Zero out
}

void interrupt_init(){
    _int_enabled = false;
    //idt = (idt_entry_t *)mem_alloc_clean(sizeof(idt_entry_t) * 256);
    //idt_ptr = (idt_ptr_t *)mem_alloc_clean(sizeof(idt_ptr_t));
    //isr_handlers = (isr_handler_t *)mem_alloc_clean(sizeof(isr_handler_t) * 256);
    //irq_handlers = (irq_handler_t *)mem_alloc_clean(sizeof(irq_handler_t) * 16);
    mem_fill((uint8 *)&idt, 0, sizeof(idt_entry_t) * 256);
    mem_fill((uint8 *)&idt_ptr, 0, sizeof(idt_ptr_t));
    mem_fill((uint8 *)isr_handlers, 0, sizeof(isr_handler_t) * 256);
    mem_fill((uint8 *)irq_handlers, 0, sizeof(irq_handler_t) * 16);
    
	idt_set_entry( 0, (uint64)isr0 , 0x8E);  // Division by zero exception
	idt_set_entry( 1, (uint64)isr1 , 0x8E);  // Debug exception
	idt_set_entry( 2, (uint64)isr2 , 0x8E);  // Non maskable (external) interrupt
	idt_set_entry( 3, (uint64)isr3 , 0x8E);  // Breakpoint exception
	idt_set_entry( 4, (uint64)isr4 , 0x8E);  // INTO instruction overflow exception
	idt_set_entry( 5, (uint64)isr5 , 0x8E);  // Out of bounds exception (BOUND instruction)
	idt_set_entry( 6, (uint64)isr6 , 0x8E);  // Invalid opcode exception
	idt_set_entry( 7, (uint64)isr7 , 0x8E);  // No coprocessor exception
	idt_set_entry( 8, (uint64)isr8 , 0x8E);  // Double fault (pushes an error code)
	idt_set_entry( 9, (uint64)isr9 , 0x8E);  // Coprocessor segment overrun
	idt_set_entry(10, (uint64)isr10, 0x8E);  // Bad TSS (pushes an error code)
	idt_set_entry(11, (uint64)isr11, 0x8E);  // Segment not present (pushes an error code)
	idt_set_entry(12, (uint64)isr12, 0x8E);  // Stack fault (pushes an error code)
	idt_set_entry(13, (uint64)isr13, 0x8E);  // General protection fault (pushes an error code)
	idt_set_entry(14, (uint64)isr14, 0x8E);  // Page fault (pushes an error code)
	idt_set_entry(15, (uint64)isr15, 0x8E);  // Reserved
	idt_set_entry(16, (uint64)isr16, 0x8E);  // FPU exception
	idt_set_entry(17, (uint64)isr17, 0x8E);  // Alignment check exception
	idt_set_entry(18, (uint64)isr18, 0x8E);  // Machine check exception
	idt_set_entry(19, (uint64)isr19, 0x8E);  // Reserved
	idt_set_entry(20, (uint64)isr20, 0x8E);  // Reserved
	idt_set_entry(21, (uint64)isr21, 0x8E);  // Reserved
	idt_set_entry(22, (uint64)isr22, 0x8E);  // Reserved
	idt_set_entry(23, (uint64)isr23, 0x8E);  // Reserved
	idt_set_entry(24, (uint64)isr24, 0x8E);  // Reserved
	idt_set_entry(25, (uint64)isr25, 0x8E);  // Reserved
	idt_set_entry(26, (uint64)isr26, 0x8E);  // Reserved
	idt_set_entry(27, (uint64)isr27, 0x8E);  // Reserved
	idt_set_entry(28, (uint64)isr28, 0x8E);  // Reserved
	idt_set_entry(29, (uint64)isr29, 0x8E);  // Reserved
	idt_set_entry(30, (uint64)isr30, 0x8E);  // Reserved
	idt_set_entry(31, (uint64)isr31, 0x8E);  // Reserved

	idt_set_entry(32, (uint64)irq0 , 0x8E);  // IRQ0 - Programmable Interrupt Timer Interrupt
	idt_set_entry(33, (uint64)irq1 , 0x8E);  // IRQ1 - Keyboard Interrupt
	idt_set_entry(34, (uint64)irq2 , 0x8E);  // IRQ2 - Cascade (used internally by the two PICs. never raised)
	idt_set_entry(35, (uint64)irq3 , 0x8E);  // IRQ3 - COM2 (if enabled)
	idt_set_entry(36, (uint64)irq4 , 0x8E);  // IRQ4 - COM1 (if enabled)
	idt_set_entry(37, (uint64)irq5 , 0x8E);  // IRQ5 - LPT2 (if enabled)
	idt_set_entry(38, (uint64)irq6 , 0x8E);  // IRQ6 - Floppy Disk
	idt_set_entry(39, (uint64)irq7 , 0x8E);  // IRQ7 - LPT1 / Unreliable "spurious" interrupt (usually)
	idt_set_entry(40, (uint64)irq8 , 0x8E);  // IRQ8 - CMOS real-time clock (if enabled)
	idt_set_entry(41, (uint64)irq9 , 0x8E);  // IRQ9 - Free for peripherals / legacy SCSI / NIC
	idt_set_entry(42, (uint64)irq10, 0x8E);  // IRQ10 - Free for peripherals / SCSI / NIC
	idt_set_entry(43, (uint64)irq11, 0x8E);  // IRQ11 - Free for peripherals / SCSI / NIC
	idt_set_entry(44, (uint64)irq12, 0x8E);  // IRQ12 - PS2 Mouse
	idt_set_entry(45, (uint64)irq13, 0x8E);  // IRQ13 - FPU / Coprocessor / Inter-processor
	idt_set_entry(46, (uint64)irq14, 0x8E);  // IRQ14 - Primary ATA Hard Disk
	idt_set_entry(47, (uint64)irq15, 0x8E);  // IRQ15 - Secondary ATA Hard Disk

	idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
	idt_ptr.base = (uint64)&idt;
    idt_set(&idt_ptr);
}

bool interrupt_status(){
    return _int_enabled;
}
void interrupt_disable(){
    if (_int_enabled){
        asm volatile("cli");
        _int_enabled = false;
    }
}
void interrupt_enable(){
    if (!_int_enabled){
        asm volatile("sti");
        _int_enabled = true;
    }
}
void interrupt_reg_isr_handler(uint64 int_no, isr_handler_t handler){
    if (int_no < 256){
		isr_handlers[int_no] = handler;
	}
}
void interrupt_reg_irq_handler(uint64 irq_no, irq_handler_t handler){
	if (irq_no < 16){
		irq_handlers[irq_no] = handler;
	}
}

static void print_stack(isr_stack_t *stack){
    debug_print(DC_WRD, "RAX: %x, RBX: %x, RCX: %x, RDX: %x", stack->rax, stack->rbx, stack->rcx, stack->rdx);
    debug_print(DC_WRD, "RDI: %x, RSI: %x, RBP: %x", stack->rdi, stack->rsi, stack->rbp);
    debug_print(DC_WRD, "CS: %x, SS: %x, RFLAGS: %x", stack->cs, stack->ss, stack->rflags);
	debug_print(DC_WRD, "RSP: %x, RIP: %x", stack->rsp, stack->rip);
}

void isr_wrapper(isr_stack_t *stack){
	uint8 int_no = (uint8)stack->int_no;
	// Try to handle interrupt
	if (isr_handlers[int_no] != 0){
		isr_handler_t handler = isr_handlers[int_no];
		if (!handler(stack)){
			return;
        }
	}

	// Process some exceptions here	
	switch (int_no){
		case 0: // Division by zero
            //stack.rip++; // it's ok to divide by zero - move to next instruction :P
#if DEBUG == 1
            debug_print(DC_WGR, "INT %d, %s", (uint64)int_no, ints[int_no]);
            debug_print(DC_WRD, "Error: %x", (uint64)stack->err_code);
            print_stack(stack);
#endif
			HANG();
			break;
		case 6: // Invalid opcode
#if DEBUG == 1
            debug_print(DC_WGR, "INT %d, %s", (uint64)int_no, ints[int_no]);
            debug_print(DC_WRD, "Error: %x", (uint64)stack->err_code);
			print_stack(stack);
#endif
			HANG();
			break;
        case 8: // Double fault
#if DEBUG == 1
            debug_print(DC_WGR, "INT %d, %s", (uint64)int_no, ints[int_no]);
            debug_print(DC_WRD, "Error: %x", (uint64)stack->err_code);
			print_stack(stack);
#endif
            HANG();
		case 13: // General protection fault
#if DEBUG == 1
            debug_print(DC_WGR, "INT %d, %s", (uint64)int_no, ints[int_no]);
            debug_print(DC_WRD, "Error: %x", (uint64)stack->err_code);
			print_stack(stack);
#endif
			HANG();
			break;
        default:
#if DEBUG == 1
            /*
	        if (int_no < 19){
		        debug_print(DC_WGR, "INT %d, %s", (uint64)int_no, ints[int_no]);
	        } else {
		        debug_print(DC_WGR, "INT %d", (uint64)int_no);
	        }
            */
#endif
            break;
	}
}

void irq_wrapper(irq_stack_t *stack){
    uint8 irq_no = (uint8)stack->irq_no;
    uint16 isr = pic_read_ocw3(PIC_READ_ISR);

    if (irq_no == 2){
        // Cascaded IRQ
        if (isr & (1 << irq_no)){
            pic_eoi(irq_no);
        }
        return;
    }
    
	if (irq_handlers[irq_no] != 0){
		irq_handler_t handler = irq_handlers[irq_no];
		if (handler(stack)){
#if DEBUG == 1
			debug_print(DC_WRD, "Unhandled IRQ %d", (uint64)irq_no);
#endif
		}
	} else {
#if DEBUG == 1
        debug_print(DC_WB, "IRQ %d", (uint64)irq_no);
#endif
    }

	// Tell PIC that it's done
    if (isr & (1 << irq_no)){
        pic_eoi(irq_no);
    }
}
