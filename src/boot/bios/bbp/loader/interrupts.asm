;
; Helper functions for operations interrupts
; ==================
;
; This contains imports from ASM stub subroutines that catch all the neccessary
; exception interrupts and calls a C function interrupt_handler().
; It also contains LIDT wrapper function.
;
; License (BSD-3)
; ===============
;
; Copyright (c) 2013, Gusts 'gusC' Kaksis <gusts.kaksis@gmail.com>
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;    * Redistributions of source code must retain the above copyright
;      notice, this list of conditions and the following disclaimer.
;    * Redistributions in binary form must reproduce the above copyright
;      notice, this list of conditions and the following disclaimer in the
;      documentation and/or other materials provided with the distribution.
;    * Neither the name of the <organization> nor the
;      names of its contributors may be used to endorse or promote products
;      derived from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
; ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;

[section .text]
[bits 64]
[extern isr_wrapper]                            ; Import isr_wrapper from C
[extern irq_wrapper]                            ; Import irq_wrapper from C
[global idt_set]                                ; Export void idt_set(idt_ptr_t *idt) to C

; Macro to push all the registers
%macro PUSH_ALL 0
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rdx
    push rcx
    push rbx
    push rax
%endmacro

; Macro to pop all the registers
%macro POP_ALL 0
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
%endmacro

; Macro to create an intterupt service routine for interrupts that do not pass error codes 
%macro INT_NO_ERR 1
[global isr%1]
isr%1:
    cli                                         ; disable interrupts
    push qword 0                                ; set error code to 0
    push qword %1                               ; set interrupt number
    push rbp                                    ; push current rbp
    PUSH_ALL                                    ; push all the registers on the stack
    mov rbp, rsp                                ; store current stack frame in rbp
    mov rdi, rsp                                ; pass our stack as a pointer to isr_wrapper
    call isr_wrapper                            ; call void isr_wrapper(int_stack_t *stack)
    POP_ALL                                     ; pop all the registers back from the stack
    pop rbp                                     ; get our rbp back from the stack
    add rsp, 16                                 ; cleanup stack
    sti                                         ; enable interrupts
    iretq                                       ; return from interrupt handler
%endmacro

; Macro to create an interrupt service routine for interrupts that DO pass an error code
%macro INT_HAS_ERR 1
[global isr%1]
isr%1:
    cli                                         ; disable interrupts
    push qword %1                               ; set interrupt number
    push rbp                                    ; push current rbp
    PUSH_ALL                                    ; push all the registers on the stack
    mov rbp, rsp                                ; store current stack frame in rbp
    mov rdi, rsp                                ; pass our stack as a pointer to isr_wrapper
    call isr_wrapper                            ; call void isr_wrapper(int_stack_t *stack)
    POP_ALL                                     ; pop all the registers back from the stack
    pop rbp                                     ; get our rbp back from the stack
    add rsp, 16                                 ; cleanup stack
    sti                                         ; enable interrupts
    iretq                                       ; return from interrupt handler
%endmacro

; Macro to create an IRQ interrupt service routine
; First parameter is the IRQ number
; Second parameter is the interrupt number it is remapped to
%macro IRQ 2
[global irq%1]
irq%1:
    cli                                         ; disable interrupts
    push qword %1                               ; set IRQ number in the place of error code (see registers_t in interrupts.h)
    push qword %2                               ; set interrupt number
    push rbp                                    ; push current rbp
    PUSH_ALL                                    ; push all the registers on the stack
    mov rbp, rsp                                ; store current stack frame in rbp
    mov rdi, rsp                                ; pass our stack as a pointer to isr_wrapper
    call irq_wrapper                            ; calls void irq_wrapper(irq_stack_t *stack)
    POP_ALL                                     ; pop all the registers back from the stack
    pop rbp                                     ; get our rbp back from the stack
    add rsp, 16                                 ; cleanup stack
    sti                                         ; enable interrupts
    iretq                                       ; return from interrupt handler
%endmacro

idt_set:                                        ; prototype: void idt_set(uint32 idt_ptr)
    lidt [rdi]                                  ; load the IDT (x86_64 calling convention - 1st argument goes into RDI)
    ret                                         ; return to C

; Setup all the neccessary service routines with macros
INT_NO_ERR 0
INT_NO_ERR 1
INT_NO_ERR 2
INT_NO_ERR 3
INT_NO_ERR 4
INT_NO_ERR 5
INT_NO_ERR 6
INT_NO_ERR 7
INT_HAS_ERR 8                                    ; By all specs int 8 is double fault WITH error message, but not in Bochs, WHY?
INT_NO_ERR 9
INT_HAS_ERR 10
INT_HAS_ERR 11
INT_HAS_ERR 12
INT_HAS_ERR 13
INT_HAS_ERR 14
INT_NO_ERR 15
INT_NO_ERR 16
INT_HAS_ERR 17
INT_NO_ERR 18
INT_NO_ERR 19
INT_NO_ERR 20
INT_NO_ERR 21
INT_NO_ERR 22
INT_NO_ERR 23
INT_NO_ERR 24
INT_NO_ERR 25
INT_NO_ERR 26
INT_NO_ERR 27
INT_NO_ERR 28
INT_NO_ERR 29
INT_NO_ERR 30
INT_NO_ERR 31
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
