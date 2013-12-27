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

#define HEAP_MAGIC 0xFFFFC0CAC01AFFFF

#define HEAP_GET_FOOTER(h) (((uint8 *)h) + ((heap_header_t *)h)->block.size + sizeof(heap_header_t))
#define HEAP_GET_HEADER(f) (((heap_footer_t *)f)->header)
#define HEAP_PAYLOAD_HEADER(p) (((uint8 *)p) - sizeof(heap_header_t))
#define HEAP_PAYLOAD_FOOTER(p) (HEAP_GET_FOOTER(HEAP_PAYLOAD_HEADER(p)))
#define HEAP_GET_PAYLOAD(h) (((uint8 *)h) + sizeof(heap_header_t))
#define HEAP_CHECK_HEADER(h) (((heap_header_t *)h)->magic == HEAP_MAGIC)
#define HEAP_CHECK_FOOTER(f) (((heap_footer_t *)f)->magic == HEAP_MAGIC && ((heap_footer_t *)f)->header->magic == HEAP_MAGIC)

/**
* Heap block header structure
*/
struct heap_header_struct {
	uint64 magic;
	struct {
		uint64 used       : 1;
		uint64 reserved   : 3; // Planned to implement locks and read/write attributes
		uint64 size : 60;
	} block;
} __PACKED;
typedef struct heap_header_struct heap_header_t;
/**
* Heap block footer structure
*/
struct heap_footer_struct {
	uint64 magic;
	heap_header_t *header;
} __PACKED;
typedef struct heap_footer_struct heap_footer_t;
/**
* Free block structure - header + pointers to next and previous blocks
*/
typedef struct free_block_struct free_block_t;
struct free_block_struct {
	heap_header_t header;
	free_block_t *prev_block;
	free_block_t *next_block;
} __PACKED;

static uint64 heap_start;
static uint64 heap_size;
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
static free_block_t *free_list[HEAP_LIST_COUNT];
// Binary search tree for blocks larger than 2048 bytes
// Single page of size 4096 can hold a payload of 4080 bytes
// next block is larger in size
// prev block is smaller in size
static free_block_t *free_tree;

/**
* Create a free block and write it's header information
* @param free_block - pointer to free block
* @param psize - payload size
*/
static void heap_create_free(free_block_t *free_block, uint64 psize){
	psize = HEAP_ALIGN(psize);

	// Set header
	free_block->header.magic = HEAP_MAGIC;
	free_block->header.block.size = psize;
	// Move to footer
	heap_footer_t *footer = (heap_footer_t *)HEAP_GET_FOOTER(free_block);
	// Set footer
	footer->header = &free_block->header;
	footer->magic = HEAP_MAGIC;
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
static free_block_t *heap_split(free_block_t *free_block, uint64 psize){
	// Caclutate leftover payload size
	psize = HEAP_ALIGN(psize);
	uint64 psize_other = free_block->header.block.size - (2 * (sizeof(heap_header_t) + sizeof(heap_footer_t))) - psize;
	if (psize_other > HEAP_LIST_MIN){
		// Split if the leftover is large enough
		free_block_t *free_other = (free_block_t *)(((uint8 *)free_block) + psize + (sizeof(heap_header_t) + sizeof(heap_footer_t)));
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
static void heap_add_free(free_block_t *free_block){
	int8 list_idx = heap_size_idx(free_block->header.block.size);
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
		} else if (free_block->header.block.size == free_tree->header.block.size){
			// Add to the right
			if (free_tree->next_block != 0){
				free_tree->next_block->prev_block = free_block;
			}
			free_block->next_block = free_tree->next_block;
			free_block->prev_block = free_tree;
			free_tree->next_block = free_block;
		} else if (free_block->header.block.size > free_tree->header.block.size){
			// Move to the right and insert the block right before the larger block
			free_block_t *free_right = free_tree;
			while (free_right->next_block != 0 && free_right->next_block->header.block.size < free_block->header.block.size){
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
			free_block_t *free_left = free_tree;
			while (free_left->prev_block != 0 && free_left->prev_block->header.block.size > free_block->header.block.size){
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
static void heap_remove_free(free_block_t *free_block){
	uint64 psize = free_block->header.block.size - (sizeof(heap_header_t) + sizeof(heap_footer_t));
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
static free_block_t *heap_find_free(uint64 psize){
	free_block_t *free_block = 0;
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
		uint64 usize = psize + (sizeof(heap_header_t) + sizeof(heap_footer_t));
	
		if (usize == free_tree->header.block.size){
			// Bingo!
			free_block = free_tree;
		} else if (usize > free_tree->header.block.size){
			// Go right
			free_block_t *free_right = free_tree;
			while (free_right->next_block != 0 && usize > free_right->next_block->header.block.size){
				free_right = free_right->next_block;
			}
			if (usize <= free_right->header.block.size){
				free_block = free_right;
			}
		} else if (usize < free_tree->header.block.size){
			// Go left
			free_block_t *free_left = free_tree;
			while (free_left->prev_block != 0 && usize < free_left->prev_block->header.block.size){
				free_left = free_left->prev_block;
			}
			if (usize <= free_left->header.block.size){
				free_block = free_left;
			} else if (free_left->next_block != 0){
				free_left = free_left->next_block;
				if (usize <= free_left->header.block.size){
					free_block = free_left;
				}
			}
		}
	}
	return free_block;
}
/**
* Extend heap size within a page size aligned size
* @param size - required payload size
* @return a new free block
*/
static free_block_t * heap_extend(uint64 psize){
	psize = HEAP_ALIGN(psize);
	uint64 usize = psize + (sizeof(heap_header_t) + sizeof(heap_footer_t));
	usize = PAGE_SIZE_ALIGN(usize);
	
	free_block_t *free_block = (free_block_t *)(((uint8 *)heap_start) + heap_size);
	uint64 page_size = usize;
	uint64 page_addr = (uint64)free_block;
	while (page_size > 0){
		page_map(page_addr);
		page_size -= PAGE_SIZE;
		page_addr += PAGE_SIZE;
	}
	heap_size += usize;

	heap_create_free(free_block, (usize - (sizeof(heap_header_t) + sizeof(heap_footer_t))));
	heap_add_free(free_block);

	return free_block;
}

void heap_init(uint64 start, uint64 isize){
	heap_start = start;
	heap_size = isize;
	// Initialize empty lists
	uint8 i = 0;
	for (i = 0; i < HEAP_LIST_COUNT; i ++){
		free_list[i] = 0;
	}
	// Initialize first block covering the whole heap
	free_tree = (free_block_t *)heap_start;
	heap_create_free(free_tree, heap_size - (sizeof(heap_header_t) + sizeof(heap_footer_t)));
}

void *heap_alloc(uint64 psize){
	free_block_t *free_block = 0;

	// Align the requested size to heap alignament
	psize = HEAP_ALIGN(psize);
	
	// Find the block in the list
	free_block = heap_find_free(psize);
	if (free_block == 0){
		// Allocate a new page if all the free space has been used
		free_block_t *free_block = heap_extend(psize);
	}
	if (free_block != 0){
		// Remove block
		heap_remove_free(free_block);

		// Split block if possible
		free_block_t *free_other = heap_split(free_block, psize);
		if (free_other != 0){
			// If there are left overs - add them to list or a tree
			heap_add_free(free_other);
		}

		// Mark used in header
		free_block->header.block.used = 1;
		// Return a pointer to the payload
		return (void *)HEAP_GET_PAYLOAD(free_block);
	}
	return 0;
}

void *heap_realloc(void *ptr, uint64 psize){
	// TODO: implement
	return 0;
}

void heap_free(void *ptr){
	free_block_t *free_block = (free_block_t *)HEAP_PAYLOAD_HEADER(ptr);
	if (HEAP_CHECK_HEADER(free_block)){
		heap_footer_t *prev_footer;
		heap_header_t *next_header;
		uint64 *ptr_b = (uint64 *)free_block;

		// Clear used in header
		free_block->header.block.used = 0;

		prev_footer = (heap_footer_t *)(((uint8 *)free_block) - sizeof(heap_footer_t));
		if (HEAP_CHECK_FOOTER(prev_footer) && !prev_footer->header->block.used){
			// Merge left
			free_block_t *free_right = (free_block_t *)HEAP_GET_HEADER(prev_footer);
			// Remove the right block
			heap_remove_free(free_right);
			// Create new block
			heap_create_free(free_right, free_right->header.block.size + free_block->header.block.size - (sizeof(heap_header_t) + sizeof(heap_footer_t)));
			// Add block to heap free lists of tree
			free_block = free_right;
		}

		next_header = (heap_header_t *)(((uint8 *)HEAP_GET_FOOTER(free_block)) + sizeof(heap_footer_t));
		if (HEAP_CHECK_HEADER(next_header) && !next_header->block.used){
			// Merge left
			free_block_t *free_left = (free_block_t *)next_header;
			// Remove the right block
			heap_remove_free(free_left);
			// Create new block
			heap_create_free(free_block, free_block->header.block.size + free_left->header.block.size - (sizeof(heap_header_t) + sizeof(heap_footer_t)));
		}

		// Add this block back to list or tree
		heap_add_free(free_block);
	}
}

#if DEBUG == 1
void heap_list(){

}
#endif
