/*

Memory heap management functions
================================

Heap management consists of free lists:
31 size segregated lists 16 - 2048 byte lists
1 binary search tree for large free blocks

Segregated by payload size as 16, 32, 64, 128, 256, 512, 1024, 2048 bytes long 
0 - 16 bytes actually use 32 bytes = header + footer of block size
1 - 17 to 32 bytes uses 48 bytes
2 - 33 to 48 -"- 64 bytes
3 - 49 to 64 -"- 80 bytes
4 - 65 to 80 -"- 96 bytes
5 - 81 to 96 -"- 112 bytes
6 - 97 to 112 -"- 128 bytes
7 - 113 to 128 -"- 144 bytes
etc.
We should'n use more lists as it would expand past page boundaries

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

// Minimum free block size listed in segregated lists in bytes (16 is a minumum on 64bit systems, 
// as we have to store 2 pointers in free list blocks, each 8 bytes)
// TODO: There's a bug if we increase this to 32 - something wrong with alignament calculations - must fix
#define HEAP_LIST_MIN 48 // 16 + sizeof(heap_header_t) + sizeof(heap_footer_t)
// Maximum free block size listed in segregated lists in bytes (larger ones go to a search tree)
#define HEAP_LIST_MAX 1056 // 1024 + sizeof(heap_header_t) + sizeof(heap_footer_t)

// Align to 16 byte boundary
#define HEAP_MASK  0xFFFFFFFFFFFFFFF0 // -16
#define HEAP_IMASK 0x000000000000000F // 15

/**
* Align address to the start of the heap alignament
* @param a - addres to align
* @return heap aligned address
*/
#define HEAP_ALIGN(a) (a & HEAP_MASK)
/**
* Align size to the minimum size of the heap
* @param s - size
* @return heap aligned size
*/
#define HEAP_SIZE_ALIGN(s) ((s + HEAP_IMASK) & HEAP_MASK)
// Element size distribution in segregated lists (let's put it the same size as minimum)
#define HEAP_LIST_SPARSE 16
// Segregated list count
#define HEAP_LIST_COUNT ((HEAP_LIST_MAX - HEAP_LIST_MIN) / HEAP_LIST_SPARSE)

/**
* Heap block size structure
*/
typedef struct {
	uint64 used         : 1;
    uint64 aligned      : 1;
	uint64 reserved     : 2; // Planned to implement locks and read/write attributes
	uint64 frames       : 60; // Upper 60 bits of heap block size, let's call them frames (to mimic micro-paging)
} heap_size_t;
/**
* Heap block header structure
*/
typedef struct packed {
	uint64 magic;
	uint64 size;
} heap_header_t;
/**
* Heap block footer structure
*/
typedef struct packed {
	uint64 magic;
	heap_header_t *header;
} heap_footer_t;
/**
* Heap structure
*/
typedef struct packed {
	uint64 start_addr;							// Start address of the heap
	uint64 end_addr;							// End address of the heap
	uint64 max_addr;							// Maximum address of the heap
    struct {
        uint64 locked               :1;         // Heap is locked
        uint64 reserved             :63;
    } flags;
	heap_header_t *free[HEAP_LIST_COUNT + 1];	// Free block list + tree in the last item
} heap_t;

/**
* Create a new heap with allocation and deallocation functionalities
* @param start - start address of the heap
* @param size - initial size of the heap
* @param max_size - maximum size of the heap
*/
heap_t * heap_create(uint64 start, uint64 size, uint64 max_size);
/**
* Allocate a block of memory on the heap
* @param heap - pointer to the beginning of the heap
* @param psize - size of a block to allocate (payload size)
* @param align - align the beginning of the block to page (4KB) boundary
* @return new pointer to the memory block allocated or 0
*/
void *heap_alloc(heap_t *heap, uint64 size, bool align);
/**
* Re allocate a block of memory on the heap
* @param heap - pointer to the beginning of the heap
* @param ptr - memory block allocated previously
* @param psize - new size of block (payload size)
* @param align - align the beginning of the block to page (4KB) boundary
* @return new pointer to the memory block allocated or 0
*/
void *heap_realloc(heap_t *heap, void *ptr, uint64 size, bool align);
/**
* Free memory block allocated by heap_alloc()
* @param heap - pointer to the beginning of the heap
* @param ptr - memory block allocated previously
*/
void heap_free(heap_t *heap, void *ptr);
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
* @param heap - pointer to the beginning of the heap
*/
void heap_list(heap_t *heap);
#endif


#endif
