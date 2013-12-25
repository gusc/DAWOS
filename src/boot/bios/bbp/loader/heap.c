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

#define HEAP_ALIGN(n) ((n + (~HEAP_ALIGN_MASK)) & HEAP_ALIGN_MASK)
#define HEAP_IS_USED(p) (*p & 1)

/**
* Free block header structure
*/
typedef struct free_header_s free_header_t;
struct free_header_s {
	uint64 block_size;
	free_header_t *prev_block;
	free_header_t *next_block;
}

static void *heap_start = (void *)HEAP_LOC;
static uint64 heap_size = HEAP_SIZE;
// Segregated by payload size as 16, 32, 64, 128, 256, 512, 1024, 2048 bytes long 
// 0 - 16 bytes actually use 32 bytes = header + footer of block size
// 1 - 17 to 32 bytes uses 48 bytes
// 2 - 33 to 48 -"- 64 bytes
// 3 - 49 to 64 -"- 80 bytes
// 4 - 65 to 80 -"- 96 bytes
// 5 - 81 to 96 -"- 112 bytes
// 6 - 97 to 112 -"- 128 bytes
// 7 - 113 to 128 -"- 144 bytes
// etc.
// We should'n use more lists as it would expand past page boundaries
static free_header_t *free_list[HEAP_LIST_COUNT];
// Binary search tree for blocks larger than 2048 bytes
// Single page of size 4096 can hold a payload of 4080 bytes
static free_header_t *free_tree;

/**
* Create a free block and write it's header information
* @param free_block - pointer to free block
* @param psize - payload size
*/
static void heap_create_free(free_header_t *free_block, uint64 psize){
	uint8 *ptr = (uint8 *)free_block;
	psize = HEAP_ALIGN(psize);

	// Set header
	free_block->block_size = psize;
	// Move to footer
	ptr += (psize + sizeof(uint64));
	// Set footer
	*((uint64 *)ptr) = psize;
}
/**
* Get list index from payload size
* @param size - payload size
* @return list index or -1 if not found
*/
static int8 heap_size_idx(uint64 psize){
	if (psize <= HEAP_LIST_MAX){
		psize = HEAP_ALIGN(psize);
		return psize / HEAP_LIST_SPARSE;
	}
	return -1;
}
/**
* Split a free block at a location and add the new block in the free list
* @param free_block - free block to split
* @param psize - new payload size
* @return newly created free block
*/
static free_header_t *heap_split(free_header_t *free_block, uint64 psize){
	// Caclutate leftover payload size
	psize = HEAP_ALIGN(psize);
	uint64 psize_other = free_block->block_size - (4 * sizeof(uint64)) - psize;
	if (psize_other > HEAP_LIST_MIN){
		// Split if the leftover is large enough
		free_header_t *free_other = (free_header_t *)(((uint8 *)free_block) + psize + (2 * sizeof(uint64)));
		// Create new block
		heap_create_free(free_other, psize_other);
		// Recreate the old block
		heap_create_free(free_block, psize);
		return free_other;
	}
	return 0;
}
/**
* Add a free block to the segregated list or a binary search tree
* @param free_block - block to add
*/
static void heap_add_free(free_header_t *free_block){
	int8 list_idx = heap_size_idx(free_block->block_size);
	if (list_idx >= 0){
		free_block->next_block = free_list[list_idx];
		if (free_block->next_block != 0){
			free_block->next_block->prev_block = free_block;
		}
		free_list[list_idx] = free_block;
	} else {
		if (free_tree == 0){
			// As easy as it can be
			free_tree = free_block;
		} else if (free_tree->block_size == free_block->block_size){
			// Add to the right
			if (free_tree->next_block != 0){
				free_tree->next_block->prev_block = free_block;
			}
			free_block->next_block = free_tree->next_block;
			free_block->prev_block = free_tree;
			free_tree->next_block = free_block;
		} else if (free_tree->block_size < free_block->block_size){
			// Move to the right and insert the block right before the larger block
			free_header_t *free_right = free_tree;
			while (free_right->next_block != 0 && free_right->next_block->block_size < free_block->block_size){
				free_right = free_right->next_block;
			}
			if (free_right->next_block != 0){
				free_right->next_block->prev_block = free_block;
				free_block->next_block = free_right->next_block;
			} else {
				free_block->next_block = 0;
			}
			free_block->prev_block = free_right;
			free_right->next_block = free_block;
		} else {
			// Move to the left and insert the block right before the smaller block
			free_header_t *free_left = free_tree;
			while (free_left->prev_block != 0 && free_left->prev_block->block_size > free_block->block_size){
				free_left = free_left->prev_block;
			}
			if (free_left->prev_block != 0){
				free_left->prev_block->next_block = free_block;
				free_block->prev_block = free_left->prev_block;
			} else {
				free_block->prev_block = 0;
			}
			free_block->next_block = free_left;
			free_left->prev_block = free_block;
		}
	}
}
/**
* Remove a free block from the segregated list or a binary search tree
* @param free_block - block to remove
*/
static void heap_remove_free(free_header_t *free_block){
	uint64 psize = free_block->block_size - (2 * sizeof(uint64));
	int8 list_idx = heap_size_idx(psize);
	if (list_idx >= 0){
		if (free_block->prev_block == 0){
			if (free_block->next_block == 0){
				// This was the last block
				free_list[list_idx] = 0;
			} else {
				// This is the top block
				free_list[list_idx] = free_block->next_block;
				free_list[list_idx]->prev_block = 0;
			}
		} else {
			// This is a block in the middle
			if (free_block->next_block != 0){
				free_block->next_block->prev_block = free_block->prev_block;
				free_block->prev_block->next_block = free_block->next_block;
			} else {
				free_block->prev_block->next_block = 0;
			}
		}
	} else {
		if (free_block->prev_block == 0){
			if (free_block->next_block == 0){
				// This one is all alone
				if (free_block == free_tree){
					free_tree = 0;
				}
			} else {
				// Left most block
				free_block->next_block->prev_block = 0;
				if (free_block == free_tree){
					free_tree = free_block->next_block;
				}
			}
		} else {
			if (free_block->next_block == 0){
				// Right most block
				free_block->prev_block->next_block = 0;
				if (free_block == free_tree){
					free_tree = free_block->prev_block;
				}
			} else {
				// Somewhere in the middle
				free_block->prev_block->next_block = free_block->next_block;
				free_block->next_block->prev_block = free_block->prev_block;
				if (free_block == free_tree){
					free_tree = free_block->next_block;
				}
			}
		}
	}
}
/**
* Find a free block from the segregated list or a binary search tree
* @param psize - required payload size
* @return a free block or 0
*/
static free_header_t *heap_find_free(uint64 psize){
	free_header_t *free_block = 0;
	psize = HEAP_ALIGN(psize);
	int8 list_idx = heap_size_idx(psize);
	if (list_idx >= 0){
		uint64 list_size = HEAP_LIST_MIN + (HEAP_LIST_SPARSE * list_idx);
		while (list_idx < HEAP_LIST_COUNT){
			if (psize <= list_size && free_list[list_idx] != 0){
				// Pop the block from the list and return
				free_block = free_list[list_idx];
				return free_block;
			}
			list_size += HEAP_LIST_SPARSE;
			list_idx ++;
		}
	}
	if (free_block == 0){
		// Get used size
		uint64 usize = psize + (2 * sizeof(uint64));
	
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
	}
	return 0;
}
/**
* Extend heap size within a page size aligned size
* @param size - required payload size
* @return a new free block
*/
static free_header_t * heap_extend(uint64 psize){
	psize = HEAP_ALIGN(psize);
	uint64 usize = psize + (2 * sizeof(uint64));
	usize = PAGE_ALIGN(usize);
	
	free_header_t *free_block = (free_header_t *)(((uint8 *)heap_start) + heap_size);
	uint64 page_size = usize;
	uint64 page_addr = (uint64)free_block;
	while (page_size > 0){
		page_map(page_addr);
		page_size -= PAGE_SIZE;
		page_addr += PAGE_SIZE;
	}

	heap_create_free(free_block, (usize - (2 * sizeof(uint64))));
	heap_add_free(free_block);

	return free_block;
}

void heap_init(){
	// Initialize empty lists
	for (uint8 i = 0; i < HEAP_LIST_COUNT; i ++){
		free_list[i] = 0;
	}
	// Initialize first block covering the whole heap
	free_tree = (free_header_t *)heap_start;
	heap_create_free(free_tree, heap_size - (2 * sizeof(uint64)));
}

void *heap_alloc(uint64 psize){
	free_header_t *free_block = 0;

	// Align the requested size to heap alignament
	psize = HEAP_ALIGN(psize);
	uint64 usize = psize + (1 * sizeof(uint64));
	
	// Find the block in the list
	free_block = heap_find_free(psize);
	if (free_block == 0){
		// Allocate a new page if all the free space has been used
		free_header_t *free_block = heap_extend(psize);
	}
	if (free_block != 0){
		// Remove block
		heap_remove_free(free_block);

		// Split block if possible
		free_header_t *free_other = heap_split(free_block, psize);
		if (free_other != 0){
			// If there are left overs - add them to list or a tree
			heap_add_free(free_other);
		}

		// Mark used in header
		free_block->block_size |= 1;
		// Mark used in footer
		*((uint64 *)(((uint8 *)free_block) + usize)) = free_block->block_size;
	}

	return (void *)free_block;
}

void *heap_realloc(void *ptr, uint64 psize){
	// TODO: implement
	return 0;
}

void heap_free(void *ptr){
	free_header_t *free_block = (free_header_t *)ptr;
	uint64 *ptr_b = (uint64 *)ptr;

	// Clear used in header
	free_block->block_size &= ((uint64)-2);
	uint64 usize = free_block->block_size;

	// Clear used in footer
	*((uint64 *)(((uint8 *)free_block) + usize)) = free_block->block_size;

	ptr_b --;
	if (HEAP_IS_USED(ptr_b) == 0){
		// Merge left
		uint64 right_size = *ptr_b;
		ptr_b = (uint64 *)(((uint8 *)ptr_b) - (right_size - sizeof(uint64)));
		free_header_t *free_right = (free_header_t *)ptr_b;
		// Remove the right block
		heap_remove_free(free_right);
		// Create new block
		heap_create_free(free_right, free_right->block_size + free_block->block_size - (2 * sizeof(uint64)));
		// Add block to heap free lists of tree
		free_block = free_right;
		ptr_b = (uint64 *)free_block;
	}

	ptr_b = (uint64 *)(((uint8 *)ptr_b) + free_block->block_size);
	if (HEAP_IS_USED(ptr_b) == 0){
		// Merge left
		uint64 left_size = *ptr_b;
		free_header_t *free_left = (free_header_t *)ptr_b;
		// Remove the right block
		heap_remove_free(free_left);
		// Create new block
		heap_create_free(free_block, free_block->block_size + free_left->block_size - (2 * sizeof(uint64)));
	}

	// Add this block back to list or tree
	heap_add_free(free_block);
}
