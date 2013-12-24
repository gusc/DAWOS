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

void *heap_start = (void *)HEAP_LOC;
uint32 heap_size = HEAP_SIZE;
// Segregated by payload size as 16, 32, 64, 128, 256, 512, 1024, 2048 bytes long 
// 0 - 16 bytes actually use 32 bytes = header + footer of block size
// 1 - 32 bytes uses 48 bytes
// 2 - 64 -"- 80 bytes
// 3 - 128 -"- 144 bytes
// 4 - 256 -"- 272 bytes
// 5 - 512 -"- 528 bytes
// 6 - 1024 -"- 1040 bytes
// 7 - 2048 -"- 2064 bytes
// We should'n use more lists as it would expand past page boundaries
free_header_t *free_list[HEAP_LIST_COUNT];
// Binary search tree for blocks larger than 2048 bytes
// Single page of size 4096 can hold a payload of 4080 bytes
free_header_t *free_tree;

/**
* Create a free block and write it's header information
* @param free - pointer to free block
* @param size - size of the free block (including headers)
* @param prev - previous free block
* @param next - next free block
*/
static void heap_create_free(free_header_t *free, uint64 size, free_header_t *prev, free_header_t *next){
	uint64 ptr = (uint64)free;
	// Header
	uint64 size_a = (size & (~HEAP_ALIGN));
	free->block_size = size_a;
	free->next_block = next;
	free->prev_block = prev;
	// Footer
	ptr += (size_a - sizeof(uint64));
	*((uint64 *)ptr) = size_a;
}
/**
* Split a free block at a location and add the new block in the free list
*/
static void heap_split_free(free_header_t *free, uint64 new_size){
	uint64 ptr = (uint64)free;
	uint64 other_size = free->block_size - new_size;
	if (other_size > HEAP_MIN_FREE){
		free_header_t *free_new = (free_header_t *)(ptr + new_size);
		heap_create_free(free_new, other_size, free->prev_block, free);
		heap_create_free(free, new_size, free_new, free->next_block);
	}
}
/**
* Get list index from payload size
* @param size - payload size
* @return list index or -1 if not found
*/
static int8 heap_size_idx(uint64 size){
	if (size <= HEAP_MAX_FREE){
		uint8 list_idx = 0;
		uint64 list_size = HEAP_MIN_FREE;
		while (list_idx < HEAP_LIST_COUNT){
			if (size <= list_size){
				return list_idx;
			}
			list_size <<= 1;
			list_idx ++;
		}
	}
	return -1;
}
/**
* Add a free block to a list
* @param free_block - block to remove
*/
static void heap_add_list(free_header_t *free_block){
	int8 list_idx = heap_size_idx(free_block->block_size);
	free_block->next_block = free_list[list_idx];
	free_list[list_idx]->prev_block = free_block;
	free_list[list_idx] = free_block; 
}
/**
* Remove a free block from a list
* @param free_block - block to remove
* @param list_idx - list index
*/
static void heap_remove_list(free_header_t *free_block, int8 list_idx){
	if (free_block->prev_block != 0){
		if (free_block->next_block != 0){
			free_block->next_block->prev_block = free_block->prev_block;
			free_block->prev_block->next_block = free_block->next_block;
		} else {
			free_block->prev_block->next_block = 0;
		}
	} else if (free_block->next_block != 0){
		// This is the top block
		free_list[list_idx] = free_block->next_block;
		free_list[list_idx]->prev_block = 0;
	} else {
		free_list[list_idx] = 0;
	}
}
/**
* Find a free block from the segregated lists
* @param size - required payload size
* @return a free block
*/
static free_header_t *heap_find_list(uint64 size){
	free_header_t *free_block = 0;
	if (size <= HEAP_MAX_FREE){
		int8 list_idx = heap_size_idx(size);
		if (list_idx >= 0){
			uint64 list_size = HEAP_MIN_FREE;
			while (list_idx < HEAP_LIST_COUNT){
				if (size <= list_size && free_list[list_idx] != 0){
					// Pop the block from the list and return
					free_block = free_list[list_idx];
					heap_remove_list(free_block, list_idx);
					// TODO: split the block if possible
					return free_block;
				}
				list_size <<= 1;
				list_idx ++;
			}
		}
	}
	return free_block;
}
/**
* Add a free block to the binary search tree of large blocks
* @param free_block - block to add
*/
static void heap_add_tree(free_header_t *free_block){

}
/**
* Remove a free block from the binary search tree of large blocks
* @param free_block - block to remove
*/
static void heap_remove_tree(free_header_t *free_block){
	if (free_block->prev_block != 0){
		if (free_block->next_block != 0){
			free_block->prev_block->next_block = free_block->next_block;
			free_block->next_block->prev_block = free_block->prev_block;
			if (free_block == free_tree){
				free_tree = free_block->next_block;
			}
		} else {
			free_block->prev_block->next_block = 0;
			if (free_block == free_tree){
				free_tree = free_block->prev_block;
			}
		}
	} else if (free_block->next_block != 0) {
		free_block->next_block->prev_block = 0;
		if (free_block == free_tree){
			free_tree = free_block->next_block;
		}
	} else {
		if (free_block == free_tree){
			free_tree = 0;
		}
	}
}
/**
* Find a free block from the binary tree of large blocks
* @param size - required payload size
* @return a free block
*/
static free_header_t *heap_find_tree(uint64 size){
	uint64 usize = size + (2 * sizeof(uint64));
	free_header_t *free_block = 0;
	if (usize == free_tree->block_size){
		// Bingo!
		free_block = free_tree;
	} else if (usize > free_tree->block_size){
		// Go right
		free_block = free_tree;
		while (free_block->block_size < usize && free_block->next_block != 0){
			free_block = free_block->next_block;
		}
	} else if (usize < free_tree->block_size){
		// Go left
		free_block = free_tree;
		while (free_block->block_size > usize && free_block->prev_block != 0){
			free_block = free_block->prev_block;
		}
		// Step one block back
		free_block = free_block->next_block;
	}
	return free_block;
}
/**
* Extend heap size within a page size aligned size
* @param size - required payload size
* @return a new free block
*/
static free_header_t * heap_extend(uint64 size){
	uint64 usize = size + (2 * sizeof(uint64));
	//TODO: allocate neccessary pages
	
	return 0;
}

void heap_init(){
	// Initialize empty lists
	for (uint8 i = 0; i < HEAP_LIST_COUNT; i ++){
		free_list[i] = 0;
	}
	// Initialize first block covering the whole heap
	free_tree = (free_header_t *)heap_start;
	heap_create_free(free_tree, heap_size, 0, 0);
}

void *heap_alloc(uint64 size){
	uint8 *ptr_b = 0;
	free_header_t *free_block = 0;

	// Align the requested size to heap alignament
	size = ((size + HEAP_ALIGN + 1) & (~HEAP_ALIGN));
	
	// Find the block in the list
	free_block = heap_find_list(size);
	if (free_block == 0){
		// Find the block in the tree
		free_block = heap_find_tree(size);
		if (free_block != 0){
			heap_remove_tree(free_block);
			// TODO: split the block if possible
		}
	}
	if (free_block == 0){
		// Allocate a new page if all the free space has been used
		free_header_t *free_block = heap_extend(size);
	}
	if (free_block != 0){
		// Process the free block
		ptr_b = (uint8 *)free_block;
		// Update block header and footer, and mark as used (1st bit set)
		ptr_b += free_block->block_size - sizeof(uint64);
		*((uint64 *)ptr_b) = (free_block->block_size | 1);
		free_block->block_size |= 1;
		ptr_b ++;
	}

	return (void *)ptr_b;
}

void heap_free(void *ptr){
	free_header_t *free_block = (free_header_t *)ptr;
	uint64 *ptr_b = (uint64 *)ptr;
	// Clear used bit
	free_block->block_size &= (~((uint64)1));
	ptr_b = (uint64 *)(((uint8 *)ptr_b) + free_block->block_size - sizeof(uint64));
	*ptr_b = free_block->block_size;

	ptr_b ++;
	if ((*ptr_b & 1) == 0){
		// Merge right
		uint64 right_size = *ptr_b;
		free_block->block_size += right_size;
		ptr_b = (uint64 *)(((uint8 *)ptr_b) + right_size - sizeof(uint64));
		*ptr_b = free_block->block_size;
	}

	ptr_b = (uint64 *)ptr;
	ptr_b --;
	if ((*ptr_b & 1) == 0){
		// Merge left
		uint64 left_size = *ptr_b;
		*ptr_b = (left_size + free_block->block_size);
		ptr_b = (uint64 *)(((uint8 *)ptr_b) - (left_size - sizeof(uint64)));
		*ptr_b = (left_size + free_block->block_size);
		// TODO: re-check if it's a list or large block

	} else {
		if (free_block->block_size <= HEAP_MAX_FREE){
			// Add to list
			heap_add_list(free_block);
		} else {
			// Add to tree
			heap_add_tree(free_block);
		}
	}
}
