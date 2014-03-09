/*

Helper functions for Model Specific Register (MSR) operations
=============================================================

This file also contains some notable MSR values defined as macros

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

#ifndef __msr_h
#define __msr_h

#include "common.h"

//
// MSR macros (Core2+)
//

#define MSR_IA32_P5_MC_ADDR 0x0
#define MSR_IA32_P5_MC_TYPE 0x1
#define MSR_IA32_APIC_BASE 0x1B
#define MSR_IA32_FEATURE_CONTROL 0x3A
#define MSR_BBL_CR_CTL3 0x11E
#define MSR_IA32_SYSENTER_CS 0x174
#define MSR_IA32_SYSENTER_ESP 0x175
#define MSR_IA32_SYSENTER_EIP 0x176
#define MSR_IA32_MISC_ENABLE 0x1A0
#define MSR_IA32_X2APIC_APICID 0x802
#define MSR_IA32_X2APIC_VERSION 0x803
#define MSR_IA32_X2APIC_TPR 0x808
#define MSR_IA32_X2APIC_PPR 0x80A
#define MSR_IA32_X2APIC_EOI 0x80B
#define MSR_IA32_X2APIC_LDR	0x80D
#define MSR_IA32_X2APIC_SIVR 0x80F
#define MSR_IA32_X2APIC_ISR0 0x810
#define MSR_IA32_X2APIC_ISR1 0x811
#define MSR_IA32_X2APIC_ISR2 0x812
#define MSR_IA32_X2APIC_ISR3 0x813
#define MSR_IA32_X2APIC_ISR4 0x814
#define MSR_IA32_X2APIC_ISR5 0x815
#define MSR_IA32_X2APIC_ISR6 0x816
#define MSR_IA32_X2APIC_ISR7 0x817
#define MSR_IA32_X2APIC_TMR0 0x818
#define MSR_IA32_X2APIC_TMR1 0x819
#define MSR_IA32_X2APIC_TMR2 0x81A
#define MSR_IA32_X2APIC_TMR3 0x81B
#define MSR_IA32_X2APIC_TMR4 0x81C
#define MSR_IA32_X2APIC_TMR5 0x81D
#define MSR_IA32_X2APIC_TMR6 0x81E
#define MSR_IA32_X2APIC_TMR7 0x81F
#define MSR_IA32_X2APIC_IRR0 0x820
#define MSR_IA32_X2APIC_IRR1 0x821
#define MSR_IA32_X2APIC_IRR2 0x822
#define MSR_IA32_X2APIC_IRR3 0x823
#define MSR_IA32_X2APIC_IRR4 0x824
#define MSR_IA32_X2APIC_IRR5 0x825
#define MSR_IA32_X2APIC_IRR6 0x826
#define MSR_IA32_X2APIC_IRR7 0x827
#define MSR_IA32_X2APIC_ESR 0x828
#define MSR_IA32_X2APIC_LVT_CMCI 0x82F
#define MSR_IA32_X2APIC_ICR 0x830
#define MSR_IA32_X2APIC_LVT_TIMER 0x832
#define MSR_IA32_X2APIC_LVT_THERMAL 0x833
#define MSR_IA32_X2APIC_LVT_PMI 0x834
#define MSR_IA32_X2APIC_LVT_LINT0 0x835
#define MSR_IA32_X2APIC_LVT_LINT1 0x836
#define MSR_IA32_X2APIC_LVT_ERROR 0x837
#define MSR_IA32_X2APIC_INIT_COUNT 0x838
#define MSR_IA32_X2APIC_CUR_COUNT 0x839
#define MSR_IA32_X2APIC_DIV_CONF 0x83E
#define MSR_IA32_X2APIC_SELF_IPI 0x83F
#define MSR_IA32_EFER 0xC0000080
#define MSR_IA32_STAR 0xC0000081
#define MSR_IA32_FMASK 0xC0000084
#define MSR_IA32_FS_BASE 0xC0000100
#define MSR_IA32_GS_BASE 0xC0000101
#define MSR_IA32_KERNEL_GS_BASE 0xC0000102

/**
* Read MSR value
* @param msr - Model Specific Register
* @param [out] val - pointer to memory location where to write MSR value
* @return void
*/
static inline void msr_read(uint32 msr, uint64 *val){
	split_uint64_t *v = (split_uint64_t *)val;
	asm volatile("rdmsr" : "=a"(v->low), "=d"(v->high) : "c"(msr));
}
/**
* Write MSR value
* @param msr - Model Specific Register
* @param [in] val - value to write into MSR
* @return void
*/
static inline void msr_write(uint32 msr, uint64 val){
	split_uint64_t *v = (split_uint64_t *)&val;
	asm volatile("wrmsr" : : "a"(v->low), "d"(v->high), "c"(msr));
}

#endif
