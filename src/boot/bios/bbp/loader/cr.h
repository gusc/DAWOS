/*

Helper function for Control Register (CR) operations
====================================================

ASM wrappers

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
#ifndef __cr_h
#define __cr_h

#include "common.h"

/**
* Read CR0
* @return uint64
*/
static inline uint64 get_cr0(){
	uint64 cr;
	asm volatile("mov %%cr0, %0" : "=a"(cr) : );
	return cr;
}
/**
* Write CR0
* @param uint64
*/
static inline void set_cr0(uint64 cr){
	asm volatile("mov %0, %%cr0" : : "r"(cr));
}

/**
* Read CR2
* @return uint64
*/
static inline uint64 get_cr2(){
	uint64 cr;
	asm volatile("mov %%cr2, %0" : "=a"(cr) : );
	return cr;
}
/**
* Write CR2
* @param uint64
*/
static inline void set_cr2(uint64 cr){
	asm volatile("mov %0, %%cr2" : : "r"(cr));
}

/**
* Read CR3
* @return uint64
*/
static inline uint64 get_cr3(){
	uint64 cr;
	asm volatile("mov %%cr3, %0" : "=a"(cr) : );
	return cr;
}
/**
* Write CR3
* @return uint64
*/
static inline void set_cr3(uint64 cr){
	asm volatile("mov %0, %%cr3" : : "r"(cr));
}

/**
* Read CR4
* @return uint64
*/
static inline uint64 get_cr4(){
	uint64 cr;
	asm volatile("mov %%cr4, %0" : "=a"(cr) : );
	return cr;
}
/**
* Write CR4
* @return uint64
*/
static inline void set_cr4(uint64 cr){
	asm volatile("mov %0, %%cr4" : : "r"(cr));
}

/**
* Read CR8
* @return uint64
*/
static inline uint64 get_cr8(){
	uint64 cr;
	asm volatile("mov %%cr8, %0" : "=a"(cr) : );
	return cr;
}
/**
* Write CR8
* @return uint64
*/
static inline void set_cr8(uint64 cr){
	asm volatile("mov %0, %%cr8" : : "r"(cr));
}

#endif
