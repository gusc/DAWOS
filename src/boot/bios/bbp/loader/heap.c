/*

Memory heap management functions
================================

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
#include "heap.h"
#include "lib.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

/**
* Free block header structure
*/
typedef struct free_header_s free_header_t;
struct free_header_s {
	uint64 block_size;
	free_header_t *prev_block;
	free_header_t *next_block;
}

void *heap_start = HEAP_LOC;
uint32 heap_size = HEAP_SIZE;
// Segregated by payload size as 16, 32, 64, 128, 256, 512, 1024, inf bytes long 
// 16 bytes actually use 32 bytes = header + (next free block address + previous free block address) + footer
// 32 bytes use 48 bytes
// 64 bytes use 80 bytes
// ...
// 1024 bytes use 1040 bytes
free_header_t *free_list[8] = {0, 0, 0, 0, 0, 0, 0, 0};

/**
* Remove a free block from a large free block list
* @param free - free block to remove
*/
static void heap_remove_free(free_header_t *free){
	if (free->next != 0){
		if (free->prev != 0){
			free->prev->next = free->next;
			free->next->prev = free->prev;
		} else {
			// This is the first free block in the list!	
			free->next->prev = 0;
			free_list[7] = free->next;
		}
	} else {
		if (free->prev != 0){
			free->prev->next = 0;
		} else {
			// This is the only free block in the list!
			free_list[7] = 0;
		}
	}
}
/**
* Create a free block and write it's header information
* @param free - pointer to free block
* @param size - size of the free block (including headers)
* @param prev - previous free block
* @param next - next free block
*/
static void heap_create_free(free_header_t *free, uint64 size, free_header_t *prev, free_header_t *next){
	void *free_i = (uint64 *)free;
	// Header
	uint64 size_a = (size & (~HEAP_ALIGN));
	free->block_size = size_a;
	free->next = next;
	free->prev = prev;
	// Footer
	free_i += size_a - sizeof(uint64);
	*((uint64 *)free_i) = size_a;
}
/**
* Split a free block at a location and add the new block in the free list
*/
static void heap_split_free(free_header_t *free, uint64 new_size){
	//heap_create_free(free_new, new_size, 0, 0);

}
/**
* Merge contigous free blocks
* @param free - free block to merge
*/
static void heap_merge_free(free_header_t *free){

}

void heap_init(){
	for (uint8 i = 0; i < 8; i ++){
		last_free[i] = 0;
	}
	last_free[7] = heap_start;
	heap_create_free(last_free[7], heap_size, 0, 0);
}

void *heap_alloc(uint64 size){
	void *ptr = 0;
	free_header_t *free_block;
	uint64 used_header_size = 2 * sizeof(uint64);
	uint64 free_header_size = sizeof(free_header_t) + sizeof(uint64);
	uint8 free_idx = 0;
	uint64 s = 16;
	while (free_idx < 7){
		if (size <= s && free_list[free_idx] != 0){
			break;
		}
		s <<= 1;
		free_idx ++;
	}
	if (free_list[free_idx] != 0){
		free_block = (free_header_t *)free_list[free_idx];
		if (free_block->next != 0){
			free_block->next->prev = 0;
			free_list[free_idx] = free_block->next;
		} else {
			free_list[free_idx] = 0;
		}
		ptr = (void *)free_block;
		ptr += sizeof(uint64);
	} else {
		free_idx = 7;
		free_block = (free_header_t *)free_list[free_idx];
		while (free_block != 0){
			if (free_block->block_size >= size + used_header_size){
				uint64 left_size = free_block->block_size - (size + used_header_size);
				if (left_size >= free_header_size + 8){
					heap_split_free(free_block, size + used_header_size);
				} else {
					heap_remove_free(free_block);
				}
				ptr = (void *)free_block;
				ptr += sizeof(uint64);
				break;
			}
		}
	}
	return ptr;
}

void heap_free(void *ptr){

}
