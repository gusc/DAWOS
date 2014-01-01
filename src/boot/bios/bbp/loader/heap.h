/*

Memory heap management functions
================================

Heap management consists of free lists:
8 size segregated lists 16 - 2048 byte lists
1 binary search tree for large free blocks

License (BSD-3)
===============

Copyright (c) 2013, Gusts 'gusC' Kaksis <gusts.kaksis@gmail.com>
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

#ifndef __heap_h
#define __heap_h

#include "common.h"

// Align to 16 byte boundary
#define HEAP_IMASK (HEAP_LIST_MIN - 1) // 15
#define HEAP_MASK  (~HEAP_IMASK) // -16

/**
* Size align to the minimum size of the heap
* @param psize - payload size
* @return heap aligned payload size
*/
#define HEAP_ALIGN(psize) ((psize + HEAP_IMASK) & HEAP_MASK)
// Element size distribution in segregated lists (let's put it the same size as minimum)
#define HEAP_LIST_SPARSE HEAP_LIST_MIN
// Segregated list count
#define HEAP_LIST_COUNT ((HEAP_LIST_MAX - HEAP_LIST_MIN) / HEAP_LIST_SPARSE)

/**
* Initialize heap
* @param start - start address of the heap
* @param isize - initial size of the heap
*/
void heap_init();
/**
* Allocate a block of memory on the heap
* @param psize - size of a block to allocate (payload size)
* @param align - align the start of the block to this size
* @return new pointer to the memory block allocated or 0
*/
void *heap_alloc_align(uint64 psize, uint64 align);
/**
* Allocate a block of memory on the heap
* @param psize - size of a block to allocate (payload size)
* @return new pointer to the memory block allocated or 0
*/
void *heap_alloc(uint64 psize);
/**
* Re allocate a block of memory on the heap
* @param ptr - memory block allocated previously
* @param psize - new size of block (payload size)
* @return new pointer to the memory block allocated or 0
*/
void *heap_realloc(void *ptr, uint64 psize);
/**
* Free memory block allocated by heal_alloc()
* @param ptr - memory block allocated previously
*/
void heap_free(void *ptr);
/**
* Get the allocated size of the pointer (alignament forces to allocate more memory than requested)
* knowing the real size might come in handy in utilizing the memory overhead
* @param ptr - memory block allocated previously
* @return the allocated size of this block
*/
uint64 heap_alloc_size(void *ptr);

#if DEBUG == 1
/**
* List heap data for debug
*/
void heap_list();
#endif


#endif
