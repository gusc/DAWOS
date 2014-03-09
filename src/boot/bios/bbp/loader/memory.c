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

#include "../config.h"
#include "memory.h"
#include "paging.h"
#include "heap.h"
#include "lib.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

/**
* The kernel heap
*/
static heap_t *_heap = 0;
/**
* Placement address
*/
static uint64 _placement_addr;

// Import placement_addr32 from 32bit mode
extern uint32 placement_addr32;

void mem_init(){
    _heap = 0;
	_placement_addr = (uint64)placement_addr32;
}
void mem_init_heap(uint64 max_size){
	_heap = heap_create(_placement_addr, PAGE_SIZE, max_size);
	_placement_addr = INIT_MEM;
}
void *mem_alloc(uint64 size){
	if (_heap != 0){
		// Heap allocation
		return heap_alloc(_heap, size, false);
	} else {
		// Simple placement address allocation
		uint64 ptr = _placement_addr;
		_placement_addr += size;
		return (void *)ptr;
	}
	return 0;
}
void *mem_alloc_align(uint64 size){
	if (_heap != 0){
		// Heap allocation
		return heap_alloc(_heap, size, true);
	} else {
		// Align the placement address to the next beginning of the page
		_placement_addr = PAGE_SIZE_ALIGN(_placement_addr);
        // Simple placement address allocation
		uint64 ptr = _placement_addr;
		_placement_addr += size;
		return (void *)ptr;
	}
	return 0;
}
void *mem_alloc_clean(uint64 size){
	if (_heap != 0){
		// Heap allocation
        void *ptr = heap_alloc(_heap, size, false);
		uint64 size = heap_alloc_size(ptr);
		mem_fill((uint8 *)ptr, 0, size);
		return ptr;
	} else {
		// Simple placement address allocation
		uint64 ptr = _placement_addr;
		mem_fill((uint8 *)ptr, 0, size);
		_placement_addr += size;
		return (void *)ptr;
	}
	return 0;
}
void *mem_alloc_ac(uint64 size){
	if (_heap != 0){
		// Heap allocation
		void *ptr = heap_alloc(_heap, size, true);
		uint64 size = heap_alloc_size(ptr);
		mem_fill((uint8 *)ptr, 0, size);
		return ptr;
	} else {
		// Align the placement address to the next beginning of the page
		_placement_addr = PAGE_SIZE_ALIGN(_placement_addr);
		// Simple placement address allocation
		uint64 ptr = _placement_addr;
		mem_fill((uint8 *)ptr, 0, size);
		_placement_addr += size;
		return (void *)ptr;
	}
	return 0;
}
void *mem_realloc(void *ptr, uint64 size){
	if (_heap != 0){
		return heap_realloc(_heap, ptr, size, false);
	}
	return 0;
}
void mem_free(void *ptr){
	if (_heap != 0){
		heap_free(_heap, ptr);
	}
}
void mem_free_clean(void *ptr){
	if (_heap != 0){
		uint64 size = heap_alloc_size(ptr);
		mem_fill((uint8 *)ptr, 0, size);
		heap_free(_heap, ptr);
	}
}


#if DEBUG == 1
void mem_list(){
	if (_heap != 0){
		heap_list(_heap);
	} else {
		debug_print(DC_WB, "Placement address: %x", _placement_addr);
	}
}
#endif