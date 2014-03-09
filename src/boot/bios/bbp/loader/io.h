/*

Helper functions for legacy IO operations
=========================================

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

#ifndef __io_h
#define __io_h

#include "common.h"

/**
* Write a byte out to the specified port.
* @param port - IO port number
* @param value - byte value
* @return void
*/
static inline void outb(uint16 port, uint8 value){
	asm volatile ("outb %1, %0" : : "d"(port), "a"(value));
}
/**
* Write a word out to the specified port.
* @param port - IO port number
* @param value - word value
* @return void
*/
static inline void outw(uint16 port, uint16 value){
	asm volatile ("outw %1, %0" : : "d"(port), "a"(value));
}
/**
* Write a dword out to the specified port.
* @param port - IO port number
* @param value - dword value
* @return void
*/
static inline void outd(uint16 port, uint32 value){
	asm volatile ("outl %1, %0" : : "d"(port), "a"(value));
}
/**
* Read a byte from specified port.
* @param port - IO port number
* @return byte value
*/
static inline uint8 inb(uint16 port){
	uint8 ret;
	asm volatile("inb %1, %0" : "=a"(ret) : "d"(port));
	return ret;
}
/**
* Read a word from specified port.
* @param port - IO port number
* @return word value
*/
static inline uint16 inw(uint16 port){
	uint16 ret;
	asm volatile("inw %1, %0" : "=a"(ret) : "d"(port));
	return ret;
}
/**
* Read a dword from specified port.
* @param port - IO port number
* @return dword value
*/
static inline uint32 ind(uint16 port){
	uint32 ret;
	asm volatile("inl %1, %0" : "=a"(ret) : "d"(port));
	return ret;
}
/**
* Read a string from a specified port one byte at a time
* @param port - IO port number
* @param address - target memory address to read into
* @param count - number of bytes to read
*/
static inline void insb(uint16 port , uint8 *address , int count){
    asm volatile("cld; rep insb" : "=D" (address), "=c" (count) : "d" (port), "0" (address), "1" (count) : "memory", "cc");
}
/**
* Read a string from a specified port one word at a time
* @param port - IO port number
* @param address - target memory address to read into
* @param count - number of words to read
*/
static inline void insw(uint16 port , uint16 *address , int count){
    asm volatile("cld; rep insw" : "=D" (address), "=c" (count) : "d" (port), "0" (address), "1" (count) : "memory", "cc");
}
/**
* Read a string from a specified port one dword at a time
* @param port - IO port number
* @param address - target memory address to read into
* @param count - number of dwords to read
*/
static inline void insd(uint16 port , uint32 *address , int count){
    asm volatile("cld; rep insl" : "=D" (address), "=c" (count) : "d" (port), "0" (address), "1" (count) : "memory", "cc");
}

#endif
