/*

Memory management functions
===========================

Loader memory management functions. First they make use of placement address, but
after you enable heap allocator with the kernel heap it's ready to make full use
of heap allocations

License (BSD-3)
===============

Copyright (c) 2014, Gusts 'gusC' Kaksis <gusts.kaksis@gmail.com>
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

#ifndef __memory_h
#define __memory_h

#include "common.h"

/**
* Initialize memory management system
*/
void mem_init();
/**
* Initialize heap allocator
*/
void mem_init_heap();
/**
* Allocate a block of memory
* @param size - required memory block size
* @return the pointer to the begining of the memory block
*/
void *mem_alloc(uint64 size);
/**
* Allocate a block of memory aligned to the page (4KB) boundary
* @param size - required memory block size
* @return the pointer to the begining of the memory block
*/
void *mem_alloc_align(uint64 size);
/**
* Allocate a block of memory and clear it's contents (zero-fill)
* @param size - required memory block size
* @return the pointer to the begining of the memory block
*/
void *mem_alloc_clean(uint64 size);
/**
* Allocate a block of memory aligned to the page (4KB) boundary and clear it's contents (zero-fill)
* @param size - required memory block size
* @return the pointer to the begining of the memory block
*/
void *mem_alloc_ac(uint64 size);
/**
* Reallocate a block of memory
* @param ptr - a pointer to the beginning of the memory block previously returned by mem_alloc functions
* @param size - the new required memory block size
* @return the pointer to the begining of the new memory block
*/
void *mem_realloc(void *ptr, uint64 size);
/**
* Release a block of memory
* @param ptr - a pointer to the beginning of the memory block previously returned by mem_alloc functions
*/
void mem_free(void *ptr);
/**
* Release a block of memory and clear it's contents (zero-fill)
* This might come in handy when dealing with some secure data that you don't want to leave as a garbage
* @param ptr - a pointer to the beginning of the memory block previously returned by mem_alloc functions
*/
void mem_free_clean(void *ptr);


#if DEBUG == 1
/**
* List heap data for debug
*/
void mem_list();
#endif

#endif /* __memory_h */
