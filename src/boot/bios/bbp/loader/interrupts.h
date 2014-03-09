/*

Helper functions for operations interrupts
==========================================

This contains imports from ASM stub subroutines that catch all the neccessary
exception interrupts and calls a C function interrupt_handler().
It also contains LIDT wrapper function.

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

#ifndef __interrupts_h
#define __interrupts_h

#include "common.h"

// Remaped IRQ numbers to interrupt numbers
#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

/**
* Stack of registers passed from assembly
*/
typedef struct {
    uint64 rax;
    uint64 rbx;
    uint64 rcx;
    uint64 rdx;
    uint64 rdi;
    uint64 rsi;
    uint64 r8;
    uint64 r9;
    uint64 r10;
    uint64 r11;
    uint64 r12;
    uint64 r13;
    uint64 r14;
    uint64 r15;
    uint64 rbp;
    uint64 int_no;				// Interrupt number
    uint64 err_code;			// Error code (or IRQ number for IRQs)
    uint64 rip;					// Return instruction pointer
    uint64 cs;					// Code segment
    uint64 rflags;				// RFLAGS
    uint64 rsp;					// Previous stack pointer
    uint64 ss;					// Stack segment
} isr_stack_t;
typedef struct {
    uint64 rax;
    uint64 rbx;
    uint64 rcx;
    uint64 rdx;
    uint64 rdi;
    uint64 rsi;
    uint64 r8;
    uint64 r9;
    uint64 r10;
    uint64 r11;
    uint64 r12;
    uint64 r13;
    uint64 r14;
    uint64 r15;
    uint64 rbp;
    uint64 int_no;				// Interrupt number
    uint64 irq_no;			    // Error code (or IRQ number for IRQs)
    uint64 rip;					// Return instruction pointer
    uint64 cs;					// Code segment
    uint64 rflags;				// RFLAGS
    uint64 rsp;					// Previous stack pointer
    uint64 ss;					// Stack segment
} irq_stack_t;
/**
* Interrupt Descriptor Table (IDT) entry
*/
typedef struct packed {
	uint16 offset_lo;			// The lower 16 bits of 32bit address to jump to when this interrupt fires
	uint16 slector;				// Kernel segment selector
    uint8 reserved1;            // Must be 0 (duh!)
    uint8 type;                 // Type attributes
	uint16 offset_hi;			// The upper 16 bits of 32bit address to jump to
	uint32 offset_64;			// The upper 32 bits of 64bit address
	uint32 reserved2;			// Reserved for 96bit systems :)
} idt_entry_t;
/**
* Interrupt service routine or request handler
* @return 1 if this interrupt is left unhandled
*/
typedef uint64(*isr_handler_t)(isr_stack_t* stack) ;
/**
* Interrupt service routine or request handler
* @return 1 if this interrupt is left unhandled
*/
typedef uint64(*irq_handler_t)(irq_stack_t* stack) ;
/**
* Interrupt Descriptor Table (IDT) pointer structure
*/
typedef struct packed{
	uint16 limit;
	uint64 base;				// The address of the first element in our idt_entry_t array.
} idt_ptr_t;
/**
* Initialize interrupt handlers
*/
void interrupt_init();
/**
* Disable interrupts
*/
extern void interrupt_disable();
/**
* Enable interrupts
*/
extern void interrupt_enable();
/**
* Get current state of interrupts
* @return interrupts enabled
*/
bool interrupt_status();
/**
* Disable all interrupts
*/
void interrupt_disable();
/**
* Enable all interrupts
*/
void interrupt_enable();
/**
* Register an interrupt service routine handler
* @param int_no - interrupt number
* @param handler - callback function
*/
void interrupt_reg_isr_handler(uint64 int_no, isr_handler_t handler);
/**
* Register an interrupt request handler
* @param int_no - interrupt number
* @param handler - callback function
*/
void interrupt_reg_irq_handler(uint64 irq_no, irq_handler_t handler);
/**
* Interrupt Service Routine (ISR) wrapper
* This will be defined in kernel code
* @param stack - registers pushed on the stack by assembly
* @return void
*/
void isr_wrapper(isr_stack_t *stack);
/**
* Interrupt Request (IRQ) wrapper
* This will be defined in kernel code
* @param stack - registers pushed on the stack by assembly
* @return void
*/
void irq_wrapper(irq_stack_t *stack);

#endif
