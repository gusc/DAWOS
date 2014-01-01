/*

Placement heap management functions
===================================

Simple placement address heap implementation

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

#include "../config.h"
#include "paging.h"
#include "pheap.h"
#include "lib.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

extern uint32 placement_addr32;
uint64 placement_addr;

/**
* Allocate a block of memory on the heap
* @param psize - size of a block to allocate (payload size)
* @param aligned - weather to align the block to page boundary
* @return new pointer to the memory block allocated or 0
*/
static void *pheap_alloc_block(uint64 psize, bool aligned){
	if (aligned){
		placement_addr = PAGE_SIZE_ALIGN(placement_addr);
	}
	uint64 tmp = placement_addr;
	placement_addr += psize;
	return (void *)tmp;
}

void pheap_init(){
	placement_addr = (uint64)placement_addr32;
}

void *pheap_alloc(uint64 psize){
	return pheap_alloc_block(psize, false);
}

void *pheap_alloc_align(uint64 psize){
	return pheap_alloc_block(psize, true);
}
