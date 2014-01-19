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
#include "memory.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

// Magic number used in heap blocks for sanity checks
#define HEAP_MAGIC 0xFFFFC0CAC01AFFFF
// Heap block header and footer size
#define HEAP_OVERHEAD (sizeof(heap_header_t) + sizeof(heap_footer_t))
// Size of the tree element data
#define HEAP_TREE_DATA_SIZE (4 * sizeof(uint64))
// Minimum size of a heap block suitable for lists
#define HEAP_ITEM_MIN_SIZE (HEAP_LIST_MIN + HEAP_OVERHEAD)
// Minimum size of a heap block suitable for search tree
#define HEAP_NODE_MIN_SIZE (HEAP_TREE_DATA_SIZE + HEAP_OVERHEAD)

/**
* Get the size of memory block from the size of the payload
* @param psize - payload size
* @return usize - used size
*/
#define HEAP_USIZE(psize) (HEAP_ALIGN(psize) + HEAP_OVERHEAD)
/**
* Get the size of the payload from the size of the block
* @param usize - used size
* @return psize - payload size
*/
#define HEAP_PSIZE(usize) (usize - HEAP_OVERHEAD)
/**
* Get the size of memory block from the header
* @param h - pointer to the header
* @return psize - block size
*/
#define HEAP_GET_USIZE(h) (((heap_header_t *)h)->size & HEAP_MASK)
/**
* Set the size of memory block in the header without affecting the flags
* @param h - pointer to the header
* @param usize - block size
*/
#define HEAP_SET_USIZE(h, usize) (((heap_header_t *)h)->size = HEAP_ALIGN(usize))
/**
* Get a pointer to the footer from header
* @param h - pointer to the header
* @return pointer to the footer
*/
#define HEAP_GET_FOOTER(h) ((heap_footer_t *)(((uint8 *)h) + HEAP_GET_USIZE(h) - sizeof(heap_footer_t)))
/**
* Get a pointer to the header from footer
* @param f - pointer to the footer
* @return pointer to the header
*/
#define HEAP_GET_HEADER(f) (((heap_footer_t *)f)->header)
/**
* Get a pointer to the header from the begining of the payload
* @param p - pointer to the payload
* @return pointer to the header
*/
#define HEAP_PAYLOAD_HEADER(p) ((heap_header_t *)(((uint8 *)p) - sizeof(heap_header_t)))
/**
* Get a pointer to the footer from the begining of the payload
* @param p - pointer to the payload
* @return pointer to the footer
*/
#define HEAP_PAYLOAD_FOOTER(p) (HEAP_GET_FOOTER(HEAP_PAYLOAD_HEADER(p)))
/**
* Get a pointer to the payload from header
* @param h - pointer to the header
* @return pointer to the payload
*/
#define HEAP_GET_PAYLOAD(h) (((uint8 *)h) + sizeof(heap_header_t))
/**
* Do the sanity check on header
* @param h - pointer to the header
* @return bool
*/
#define HEAP_CHECK_HEADER(h) (((heap_header_t *)h)->magic == HEAP_MAGIC && HEAP_GET_FOOTER(h)->magic == HEAP_MAGIC)
/**
* Do the sanity check on footer
* @param f - pointer to the footer
* @return bool
*/
#define HEAP_CHECK_FOOTER(f) (((heap_footer_t *)f)->magic == HEAP_MAGIC && ((heap_footer_t *)f)->header->magic == HEAP_MAGIC)
/**
* Calculate list index from the size
* @param s - payload size
* @return idx - list index
*/
#define HEAP_SIZE_IDX(s) (s <= HEAP_LIST_MAX ? (HEAP_ALIGN(s) / HEAP_LIST_SPARSE) : -1);
/**
* Test if the payload is aligned to page boundary
* @param p - pointer to the payload
* @return true if the payload is aligned
*/
#define HEAP_IS_PAGE_ALIGNED(p) (!(((uint64)p) & PAGE_IMASK))
/**
* Test if the block is used
* @param h - pointer to header
* @return true if block is used
*/
#define HEAP_GET_USED(h) (((heap_size_t *)&h->size)->used)
/**
* Set block used state
* @param h - pointer to header
* @param u - 1 (used) or 0 (unused)
*/
#define HEAP_SET_USED(h, u) (((heap_size_t *)&h->size)->used = u)

/**
* Free block structure for free lists - header + pointers to next and previous blocks
*/
typedef struct free_item_struct free_item_t;
struct free_item_struct {
	heap_header_t header;
	free_item_t *prev_block; // Previous block in the segregated list
	free_item_t *next_block; // Next block in the segregated list
} __PACKED;
/**
* Free block structure for search tree - header + pointers to next and previous blocks
*/
typedef struct free_node_struct free_node_t;
struct free_node_struct {
	heap_header_t header;
	free_node_t *smaller_block; // Smaller block in the sorted list
	free_node_t *larger_block; // Larger block in the sorted list
	free_node_t *child_block; // Equaly sized child block
	free_node_t *parent_block; // Parent block of equaly sized child
} __PACKED;

/**
* Create a heap block. Write header and footer information.
* @param ptr - free memory location on which to build a heap block
* @param psize - payload size
*/
static void heap_create_block(void *ptr, uint64 psize){
	psize = HEAP_ALIGN(psize);

	// Cast this void space to a header of the heap block 
	heap_header_t *header = (heap_header_t *)ptr;

	// Set header
	header->magic = HEAP_MAGIC;
	header->size = psize + HEAP_OVERHEAD;
	
	// Move to footer
	heap_footer_t *footer = HEAP_GET_FOOTER(header);

	// Set footer
	footer->header = header;
	footer->magic = HEAP_MAGIC;
}
/**
* Split a block at a spcific location (relative to payload size)
* @param block - block to split
* @param psize - new payload size
* @return newly created block
*/
static heap_header_t *heap_split_block(heap_header_t *block, uint64 psize){
	// Caclutate leftover payload size
	psize = HEAP_ALIGN(psize);
	uint64 usize = HEAP_GET_USIZE(block);
	if (psize + (2 * HEAP_OVERHEAD) + HEAP_LIST_MIN <= usize){
		uint64 psize2 = usize - (2 * HEAP_OVERHEAD) - psize;
		// Split if the leftover is large enough
		heap_header_t *block2 = (heap_header_t *)(((uint8 *)block) + psize + HEAP_OVERHEAD);
		// Create a new block
		heap_create_block(block2, psize2);
		// Recreate the old block
		heap_create_block(block, psize);
		return block2;
	}
	return 0;
}

/**
* Insert a free block into a sorted list
* @param tree - a pointer to a root of the search tree
* @param block - block to add
* @return true on success, false if the block was too small for tree structure
*/
static bool heap_tree_insert(free_node_t **tree, heap_header_t *block){
	if (HEAP_GET_USIZE(block) > HEAP_OVERHEAD + HEAP_TREE_DATA_SIZE){
		free_node_t *free_block = (free_node_t *)block;
		if ((*tree) == 0){
			// As easy as it can be
			(*tree) = free_block;
			(*tree)->parent_block = 0;
			(*tree)->child_block = 0;
			(*tree)->smaller_block = 0;
			(*tree)->larger_block = 0;
			return true;
		} else {
			uint64 usize = HEAP_GET_USIZE(free_block);
			free_node_t *block = (*tree);
			free_node_t *parent_prev = 0;
			uint64 block_usize;
			bool set = false;
			while (!set){
				block_usize = HEAP_GET_USIZE(block);
				if (block_usize > usize){
					// Smaller than selected
					if (block->smaller_block == 0){
						// No more where to go - store here
						block->smaller_block = free_block;
						free_block->larger_block = block;
						free_block->parent_block = 0;
						free_block->child_block = 0;
						return true;
					} else if (HEAP_GET_USIZE(block->smaller_block) < usize){
						// Insert inbetween if the next one is smaller
						free_block->smaller_block = block->smaller_block;
						block->smaller_block->larger_block = free_block;
						block->smaller_block = free_block;
						free_block->larger_block = block;
						free_block->parent_block = 0;
						free_block->child_block = 0;
						return true;
					}
					// Move to next smaller block
					block = block->smaller_block;
				} else if (block_usize < usize){
					// Larger than selected
					if (block->larger_block == 0){
						// No more where to go - store here
						free_block->larger_block = 0;
						block->larger_block = free_block;
						free_block->smaller_block = block;
						free_block->parent_block = 0;
						free_block->child_block = 0;
						return true;
					} else if (HEAP_GET_USIZE(block->larger_block) > usize){
						// Insert inbetween if the next one is larger
						free_block->larger_block = block->larger_block;
						block->larger_block->smaller_block = free_block;
						block->larger_block = free_block;
						free_block->smaller_block = block;
						free_block->parent_block = 0;
						free_block->child_block = 0;
						return true;
					}
					// Move to next larger block
					block = block->larger_block;
				} else {
					// Equal in size - push into a child list
					if (block->child_block == 0){
						// Parent has no equal block stored
						block->child_block = free_block;
						free_block->parent_block = block;
						free_block->child_block = 0;
						free_block->smaller_block = 0;
						free_block->larger_block = 0;
						return true;
					} else {
						// Insert before the last child
						block->child_block->parent_block = free_block;
						free_block->child_block = block->child_block;
						block->child_block = free_block;
						free_block->parent_block = block;
						free_block->smaller_block = 0;
						free_block->larger_block = 0;
						return true;
					}
				}
			}
		}
	}
	return false;
}
/**
* Delete a block from search tree
* @param tree - a pointer to a root of the search tree
* @param block - block to remove
*/
static void heap_tree_delete(free_node_t **tree, heap_header_t *block){
	free_node_t *free_block = (free_node_t *)block;
	if (free_block->parent_block != 0) {
		// This is a child item
		// Simply remove this block from it's parent
		// There should never be any larger or smaller blocks for childs
		free_block->parent_block->child_block = 0;
	} else {
		free_node_t *replacement;
		if (free_block->child_block != 0){
			// We have an equal child, just copy branches and move on
			if (free_block->larger_block != 0){
				free_block->larger_block->smaller_block = free_block->child_block;
				free_block->child_block->larger_block = free_block->larger_block;
			} else {
				free_block->child_block->larger_block = 0;
			}
			if (free_block->smaller_block != 0){
				free_block->smaller_block->larger_block = free_block->child_block;
				free_block->child_block->smaller_block = free_block->smaller_block;
			} else {
				free_block->child_block->smaller_block = 0;
			}
			if (free_block->parent_block != 0){
				free_block->child_block->parent_block = free_block->parent_block;
				free_block->parent_block->child_block = free_block->child_block;
			}
			replacement = free_block->child_block;
		} else  {
			// Remove from the list
			if (free_block->larger_block != 0){
				free_block->larger_block->smaller_block = free_block->smaller_block;
				replacement = free_block->larger_block;
			}
			if (free_block->smaller_block != 0){
				free_block->smaller_block->larger_block = free_block->larger_block;
				replacement = free_block->smaller_block;
			}
		}
		if ((*tree) == free_block){
			// Replace the root with something
			(*tree) = replacement;
		}
	}
}
/**
* Search for a free block that matches size and alignament criteria
* @param tree - a pointer to a root of the search tree
* @param psize - payload size
* @param align - align the block to page boundary
* @return a pointer to heap block header
*/
static heap_header_t *heap_tree_search(free_node_t **tree, uint64 psize, bool align){
	uint64 usize = HEAP_USIZE(psize);
	free_node_t *free_block = (*tree);

	// Move our search pointer to the first suitable entry
	if (HEAP_GET_USIZE(free_block) < usize){
		while (free_block != 0 && HEAP_GET_USIZE(free_block) < usize){
			free_block = free_block->larger_block;
		}
	} else if (HEAP_GET_USIZE(free_block) > usize) {
		while (HEAP_GET_USIZE(free_block) > usize){
			if (free_block->smaller_block != 0 && HEAP_GET_USIZE(free_block->smaller_block) >= usize){
				free_block = free_block->smaller_block;
			} else {
				break;
			}
		}
	}
	if (!align){
		// We might have found something for an unaligned request (it might be 0 too)
		return (heap_header_t *)free_block;
	} else {
		// Try to get a well aligned space or at least something that can be split appart
		while (free_block != 0){
			free_node_t *block = free_block;
			if (HEAP_GET_USIZE(free_block) == usize){
				// Equal size block found
				while (block != 0){
					if (!align || (align && HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(block)))){
						// Perfect match
						// * weather just the size is matching
						// * or it's size matched and well aligned
						return (heap_header_t *)block;
					}
					// Move to equaly sized child node
					block = block->child_block;
				}
				// Fat chance - move to larger blocks instead
				free_block = free_block->larger_block;
			} else {
				// Larger block found
				while (block != 0){
					if (HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(block))){
						// Perfect match
						return (heap_header_t *)block;
					} else {
						// Test if we can split it at the boundary
						uint64 baddr = (uint64)block;
						uint64 busize = HEAP_GET_USIZE(block);
						uint64 paddr = (uint64)HEAP_GET_PAYLOAD(block);
						// Calculate aligned payload address
						uint64 paddr_align = PAGE_SIZE_ALIGN(paddr);
						// Calculate the block address after alignament
						uint64 baddr_align = paddr_align - HEAP_OVERHEAD;
						// Calculate leftover block used size
						uint64 left_usize = baddr_align - baddr;
						// Calculate used size of the aligned block
						uint64 busize_align = busize - left_usize;
						if (left_usize > HEAP_ITEM_MIN_SIZE && busize_align >= usize){
							// Everything seems fine:
							// * leftover can be split of
							// * the block will suite fine after alignament
							return (heap_header_t *)block;
						}
					}
					// Move to the child block
					block = block->child_block;
				}
				// Move to larger block
				free_block = free_block->larger_block;
			}
		}
	}
	return 0;
}

/**
* Push a free block to the segregated list
* @param list - a pointer to the free lists
* @param block - block to add
* @return true on success, false if the block was too large for free lists
*/
static bool heap_list_push(free_item_t **list, heap_header_t *block){
	uint64 usize = HEAP_GET_USIZE(block);
	uint64 psize = usize - HEAP_OVERHEAD;
	int8 list_idx = HEAP_SIZE_IDX(psize);
	free_item_t *free_block = (free_item_t *)block;
	if (list_idx >= 0){
		// Add to the list
		free_block->next_block = list[list_idx];
		if (free_block->next_block != 0){
			free_block->next_block->prev_block = free_block;
		}
		list[list_idx] = free_block;
		return true;
	}
	return false;
}
/**
* Remove a free block from a the segregated list
* @param heap - pointer to the free lists
* @param bloc - block to remove
* @return true on success
*/
static bool heap_list_pop(free_item_t **list, heap_header_t *block){
	uint64 usize = HEAP_GET_USIZE(block);
	uint64 psize = usize - HEAP_OVERHEAD;
	int8 list_idx = HEAP_SIZE_IDX(psize);
	free_item_t *free_block = (free_item_t *)block;
	if (list_idx >= 0){
		if (free_block->prev_block == 0){
			if (free_block->next_block == 0){
				// This was the only block on the lists
				list[list_idx] = 0;
			} else {
				// This is the top block
				list[list_idx] = free_block->next_block;
				list[list_idx]->prev_block = 0;
			}
		} else {
			if (free_block->next_block != 0){
				// This is a block in the middle
				free_block->next_block->prev_block = free_block->prev_block;
				free_block->prev_block->next_block = free_block->next_block;
			} else {
				// This is the bottom block
				free_block->prev_block->next_block = 0;
			}
		}
		return true;
	}
	return false;
}
/**
* Find a free list in the free block list
* @param heap - pointer to the beginning of the heap
* @param psize - payload size
* @param align - align the block to a page boundary
* @return a free block
*/
static heap_header_t *heap_list_search(free_item_t **list, uint64 psize, bool align){
	int8 list_idx = HEAP_SIZE_IDX(psize);
	if (list_idx >= 0){
		uint64 list_size = HEAP_LIST_MIN + (HEAP_LIST_SPARSE * list_idx);
		while (list_idx < HEAP_LIST_COUNT){
			if (list[list_idx] != 0){
				free_item_t *free_block = list[list_idx];
				if (!align || HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(free_block))){
					// No alignament required or this block is just perfect
					free_block = list[list_idx];
					return (heap_header_t *)free_block;
				} else {
					// Try to find one that's well aligned in the same list
					uint64 payload_loc = 0;
					uint64 payload_off = 0;
					while (free_block->next_block != 0){
						payload_loc = (uint64)HEAP_GET_PAYLOAD(free_block);							
						if (HEAP_IS_PAGE_ALIGNED(payload_loc)){
							// This one is perfect
							return (heap_header_t *)free_block;
						} else {
							// We have to test if we can try to align it
							payload_off = PAGE_SIZE - ((payload_loc + sizeof(heap_header_t)) % PAGE_SIZE);
							if (list_size >= psize + payload_off){
								return (heap_header_t *)free_block;
							}
						}
						// Move to the next one in the same list
						free_block = free_block->next_block;
					}
				}
			}
			// Move to the next list
			list_size += HEAP_LIST_SPARSE;
			list_idx ++;
		}
	}
	return 0;
}
/**
* Merge block to the left
* @param free_block - the block you try to merge
* @return same block or the left one merged with the one you passed
*/
static heap_header_t * heap_merge_left(heap_t *heap, heap_header_t *block){
	if (((uint64)block) > heap->start_addr){
		heap_footer_t *prev_footer = (heap_footer_t *)(((uint8 *)block) - sizeof(heap_footer_t));
		if (HEAP_CHECK_FOOTER(prev_footer) && HEAP_GET_USED(prev_footer->header) == 0){
			// Merge left
			heap_header_t *free_left = (heap_header_t *)HEAP_GET_HEADER(prev_footer);
			// Remove the right block
			if (!heap_list_pop((free_item_t **)heap->free_list, free_left)){
				heap_tree_delete((free_node_t **)&heap->free_tree, free_left);
			}
			// Create new block
			heap_create_block(free_left, HEAP_GET_USIZE(free_left) + HEAP_GET_USIZE(block) - HEAP_OVERHEAD);
			// Add block to heap free lists of tree
			block = free_left;
		}
	}
	return block;
}
/**
* Merge block to the right
* @param free_block - the block you try to merge
*/
static void heap_merge_right(heap_t *heap, heap_header_t *block){
	heap_footer_t *footer = (heap_footer_t *)HEAP_GET_FOOTER(block);
	if (((uint64)footer) + sizeof(heap_footer_t) < heap->end_addr){
		heap_header_t *next_header = (heap_header_t *)(((uint8 *)footer) + sizeof(heap_footer_t));
		if (HEAP_CHECK_HEADER(next_header) && HEAP_GET_USED(next_header) == 0){
			// Merge left
			heap_header_t *free_right = (heap_header_t *)next_header;
			// Remove the right block
			if (!heap_list_pop((free_item_t **)heap->free_list, free_right)){
				heap_tree_delete((free_node_t **)&heap->free_tree, free_right);
			}
			// Create new block
			heap_create_block(block, HEAP_GET_USIZE(block) + HEAP_GET_USIZE(free_right) - HEAP_OVERHEAD);
		}
	}
}
/**
* Extend heap size within a page size aligned size
* @param heap - pointer to the beginning of the heap
* @param size - required payload size
* @return a new free block
*/
static heap_header_t * heap_extend(heap_t *heap, uint64 psize){
	psize = HEAP_ALIGN(psize);
	uint64 usize = psize + HEAP_OVERHEAD;
	uint64 page_size = PAGE_SIZE_ALIGN(usize);
	heap_header_t *block = (heap_header_t *)(heap->end_addr);
	
	// Allocate pages
	while (page_size > 0){
		heap->end_addr += PAGE_SIZE;
		page_size -= PAGE_SIZE;
	}

	// Create a new free block
	heap_create_block(block, usize - HEAP_OVERHEAD);
	// Try to merge with the last one
	block = heap_merge_left(heap, block);
	
	return block;
}

heap_t * heap_create(uint64 start, uint64 size, uint64 max_size){
	// Inception kind of thing :)
	heap_t *heap = (heap_t *)start;
	uint64 psize = size - sizeof(heap_t);
	
	heap->start_addr = start;
	heap->end_addr = start + size;
	heap->max_addr = start + max_size;

	// Initialize free lists
	uint8 i = 0;
	for (i = 0; i < HEAP_LIST_COUNT; i ++){
		heap->free_list[i] = 0;
	}

	// Initialize free tree
	heap->free_tree = (heap_header_t *)(heap->start_addr + sizeof(heap_t));

	// Initialize first block in the free tree
	heap_create_block(heap->free_tree, psize - HEAP_OVERHEAD);

	return heap;
}

void *heap_alloc(heap_t * heap, uint64 psize, bool align){
	heap_header_t *free_block = 0;

	// Align the requested size to heap alignament
	psize = HEAP_ALIGN(psize);
	
	// Find the block in the list
	free_block = heap_list_search((free_item_t **)heap->free_list, psize, align);
	if (free_block == 0){
		// Didn't find it - look up in the tree
		free_block = heap_tree_search((free_node_t **)&heap->free_tree, psize, align);	
	}
	if (free_block == 0){
		// Allocate a new page if all the free space has been used
		free_block = heap_extend(heap, psize);
	} else {
		// Remove block
		if (!heap_list_pop((free_item_t **)heap->free_list, free_block)){
			heap_tree_delete((free_node_t **)&heap->free_tree, free_block);
		}
	}
	if (free_block != 0){
		// Split block if possible
		heap_header_t *free_other;
		if (align){
			uint64 baddr = (uint64)free_block;
			uint64 busize = HEAP_GET_USIZE(free_block);
			uint64 paddr = (uint64)HEAP_GET_PAYLOAD(free_block);
			// Calculate aligned payload address
			uint64 paddr_align = PAGE_SIZE_ALIGN(paddr);
			// Calculate the block address after alignament
			uint64 baddr_align = paddr_align - HEAP_OVERHEAD;
			// Calculate leftover block used size
			uint64 left_usize = baddr_align - baddr;
			free_other = heap_split_block(free_block,  HEAP_PSIZE(left_usize));
			// Swap pointers
			heap_header_t *free = free_block;
			free_block = free_other;
			free_other = free;
		} else {
			free_other = heap_split_block(free_block, psize);
		}
		if (free_other != 0){
			// If there are left overs - add them to list or a tree
			debug_print(DC_WB, "Leftover @%x", (uint64)free_other);
			BREAK();
			if (!heap_list_push((free_item_t **)heap->free_list, free_other)){
				heap_tree_insert((free_node_t **)&heap->free_tree, free_other);
			}
		}

		// Mark used in header
		HEAP_SET_USED(free_block, 1);
		// Return a pointer to the payload
		return (void *)HEAP_GET_PAYLOAD(free_block);
	}
	return 0;
}

void *heap_realloc(heap_t * heap, void *ptr, uint64 psize, bool align){
	uint64 psize_now = heap_alloc_size(ptr);
	if (psize_now > psize){
		// For now, but we should check if it can be splitted and some parts freed
		return ptr;
	} else {
		// This is easy :)
		void *ptr_new = heap_alloc(heap, psize, align);
		mem_copy((uint8 *)ptr_new, psize_now, (uint8 *)ptr);
		heap_free(heap, ptr);
		return ptr_new;
	}
}

void heap_free(heap_t * heap, void *ptr){
	if (ptr != 0){
		heap_header_t *free_block = (heap_header_t *)HEAP_PAYLOAD_HEADER(ptr);
		if (HEAP_CHECK_HEADER(free_block)){
			// Clear used in header
			HEAP_SET_USED(free_block, 0);

			// Try to merge on the left side
			free_block = heap_merge_left(heap, free_block);
		
			// Try to merge on the right side
			heap_merge_right(heap, free_block);

			// Add this block back to list or tree
			if (!heap_list_push((free_item_t **)heap->free_list, free_block)){
				heap_tree_insert((free_node_t **)&heap->free_tree, free_block);
			}
		}
	}
}

uint64 heap_alloc_size(void *ptr){
	if (ptr != 0){
		heap_header_t *free_block = (heap_header_t *)HEAP_PAYLOAD_HEADER(ptr);
		if (HEAP_CHECK_HEADER(free_block)){
			return HEAP_PSIZE(HEAP_GET_USIZE(free_block));
		}
	}
	return 0;
}

#if DEBUG == 1
void heap_list(heap_t *heap){
	int8 i = 0;
	uint64 count = 0;
	free_item_t *item;
	for (i = 0; i < HEAP_LIST_COUNT; i ++){
		if (heap->free_list[i] != 0){
			debug_print(DC_WB, "List %d", (int64)i);
			item = (free_item_t *)heap->free_list[i];
			count = 1;
			while (item->next_block != 0){
				count ++;
				item = item->next_block;
			}
			debug_print(DC_WB, "    last item @%x", (uint64)heap->free_list[i]);
			debug_print(DC_WB, "    total items %d", count);
		}
	}
	if (heap->free_tree != 0){
		debug_print(DC_WB, "Tree last item @%x size %d", (uint64)heap->free_tree, (uint64)HEAP_GET_USIZE(heap->free_tree));
	}
}
#endif
