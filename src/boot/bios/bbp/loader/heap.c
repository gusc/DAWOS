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

// Magic number used in heap blocks for sanity checks
#define HEAP_MAGIC 0xFFFFC0CAC01AFFFF
// Heap block header and footer size
#define HEAP_OVERHEAD (sizeof(heap_header_t) + sizeof(heap_footer_t))
// Size of the tree element data
#define HEAP_TREE_DATA_SIZE (sizeof(free_node_t))

/**
* Convert payload size to block size
* @param psize - payload size
* @return block size
*/
#define HEAP_BSIZE(psize) (HEAP_SIZE_ALIGN(psize) + HEAP_OVERHEAD)
/**
* Convert block size to payload size
* @param bsize - block size
* @return payload size
*/
#define HEAP_PSIZE(bsize) (HEAP_ALIGN(bsize) - HEAP_OVERHEAD)
/**
* Get the block size
* @param h - pointer to the header
* @return block size
*/
#define HEAP_GET_SIZE(h) (HEAP_ALIGN(((heap_header_t *)h)->size))
/**
* Set the block size without affecting the flags
* @param h - pointer to the header
* @param block size
*/
#define HEAP_SET_SIZE(h, bsize) (((heap_header_t *)h)->size = (HEAP_ALIGN(usize) | (((heap_header_t *)h)->size & HEAP_IMASK)))
/**
* Get a pointer to the footer from header
* @param h - pointer to the header
* @return pointer to the footer
*/
#define HEAP_GET_FOOTER(h) ((heap_footer_t *)(((uint64)h) + HEAP_GET_SIZE(h) - sizeof(heap_footer_t)))
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
#define HEAP_PAYLOAD_HEADER(p) ((heap_header_t *)(((uint64)p) - sizeof(heap_header_t)))
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
#define HEAP_GET_PAYLOAD(h) ((void *)(((uint64)h) + sizeof(heap_header_t)))
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
#define HEAP_CHECK_FOOTER(f) (((heap_footer_t *)f)->magic == HEAP_MAGIC && HEAP_GET_HEADER(f)->magic == HEAP_MAGIC)
/**
* Calculate list index from the size
* @param s - payload size
* @return idx - list index
*/
#define HEAP_SIZE_IDX(s) (s <= HEAP_LIST_MAX ? (HEAP_ALIGN(s - HEAP_OVERHEAD) / HEAP_LIST_SPARSE) : -1);
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
* Get the header of the block to the left
* @param h - pointer to header
* @return left block header
*/
#define HEAP_LEFT_FOOTER(h) ((heap_footer_t *)(((uint64)h) - sizeof(heap_footer_t)))
/**
* Get the header of the block to the right
* @param h - pointer to header
* @return right block header
*/
#define HEAP_RIGHT_HEADER(h) ((heap_header_t *)(((uint64)h) + HEAP_GET_SIZE(h)))

/**
* Wait for the heap to be unlocked
* @param heap - pointer to the heap structure
*/
static void heap_wait_lock(heap_t *heap){
    uint64 i = 0;
    while (heap->flags.locked){
        i ++;
    }
    heap->flags.locked = 1;
}
/**
* Unlock the heap
* @param heap - pointer to the heap structure
*/
static void heap_unlock(heap_t *heap){
    heap->flags.locked = 0;
}

/**
* Insert a free block into a sorted list
* @param tree - a pointer to a root of the search tree
* @param block - block to add
* @return true on success, false if the block was too small for tree structure
*/
static bool heap_tree_insert(free_node_t **tree, heap_header_t *block){
	uint64 bsize = HEAP_GET_SIZE(block);
    if (bsize >= HEAP_TREE_DATA_SIZE){
		free_node_t *free_block = (free_node_t *)block;
		// Clear pointers
		free_block->parent_block = 0;
		free_block->child_block = 0;
		free_block->smaller_block = 0;
		free_block->larger_block = 0;
		if ((*tree) == 0){
			// As easy as it can be - the tree is empty, we make the current block a root
			(*tree) = free_block;
			return true;
		} else {
			free_node_t *parent_block = (*tree);
			free_node_t *parent_prev = 0;
			uint64 block_size;
			bool set = false;
			while (!set){
				block_size = HEAP_GET_SIZE(parent_block);
				if (block_size > bsize){
					// Smaller than selected
					if (parent_block->smaller_block == 0){
						// No more where to go - store here
						parent_block->smaller_block = free_block;
						free_block->larger_block = parent_block;
						return true;
					} else if (HEAP_GET_SIZE(parent_block->smaller_block) < bsize){
						// Insert inbetween if the next one is smaller
						free_block->smaller_block = parent_block->smaller_block;
						parent_block->smaller_block->larger_block = free_block;
						parent_block->smaller_block = free_block;
						free_block->larger_block = parent_block;
						return true;
					}
					// Move to next smaller block
					parent_block = parent_block->smaller_block;
				} else if (block_size < bsize){
					// Larger than selected
					if (parent_block->larger_block == 0){
						// No more where to go - store here
						parent_block->larger_block = free_block;
						free_block->smaller_block = parent_block;
						return true;
					} else if (HEAP_GET_SIZE(parent_block->larger_block) > bsize){
						// Insert inbetween if the next one is larger
						free_block->larger_block = parent_block->larger_block;
						parent_block->larger_block->smaller_block = free_block;
						parent_block->larger_block = free_block;
						free_block->smaller_block = parent_block;
						return true;
					}
					// Move to next larger block
					parent_block = parent_block->larger_block;
				} else {
					// Equal in size - push into a child list
					if (parent_block->child_block != 0){
						// Push before the last child
						parent_block->child_block->parent_block = free_block;
						free_block->child_block = parent_block->child_block;
                    }
                    // Store current block as a parent's child
					parent_block->child_block = free_block;
					free_block->parent_block = parent_block;
					return true;
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
		free_block->parent_block->child_block = free_block->child_block;
        if (free_block->child_block != 0){
            // Set child's parent to current blocks parent
            free_block->child_block->parent_block = free_block->parent_block;
        }
	} else {
		free_node_t *replacement = 0;
		if (free_block->child_block != 0){
			// We have an equal child, just copy branches and move on
            // Copy larger block pointers
			if (free_block->larger_block != 0){
				free_block->larger_block->smaller_block = free_block->child_block;
			}
            free_block->child_block->larger_block = free_block->larger_block;
            // Copy smaller block pointers
			if (free_block->smaller_block != 0){
				free_block->smaller_block->larger_block = free_block->child_block;
			}
            free_block->child_block->smaller_block = free_block->smaller_block;
			// The child is the replacement
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
	// Clear pointers
	free_block->child_block = 0;
	free_block->parent_block = 0;
	free_block->larger_block = 0;
	free_block->smaller_block = 0;
}
/**
* Search for a free block that matches size and alignament criteria
* @param tree - a pointer to a root of the search tree
* @param bsize - block size
* @return a pointer to heap block header
*/
static heap_header_t *heap_tree_search_aligned(free_node_t **tree, uint64 bsize){
    free_node_t *free_block = (*tree);
    // Return - there's nothing to search into
    if (free_block == 0){
        return 0;
    }
    // Move our search pointer to the first suitable entry
    // First we test for a larger block if current one is too small
    while (HEAP_GET_SIZE(free_block) < bsize && free_block->larger_block != 0){
		free_block = free_block->larger_block;
	}
    // Return - the block is too small and there are no more larger blocks left
    if (HEAP_GET_SIZE(free_block) < bsize){
        return 0;
    }
	// Try to get a well aligned space or at least something that can be split appart
	while (free_block != 0){
		free_node_t *block = free_block;
		// Larger block found
		while (block != 0){
			if (HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(block))){
				// Perfect match
				return (heap_header_t *)block;
			} else {
				// Test if we can split it at the boundary
				uint64 block_addr = (uint64)block;
				uint64 block_size = HEAP_GET_SIZE(block);
				uint64 payload_addr = (uint64)HEAP_GET_PAYLOAD(block);
				// Calculate aligned payload address
				uint64 payload_offset = PAGE_SIZE_ALIGN(payload_addr);
				// Calculate the block address after alignament
				uint64 block_offset = payload_offset - sizeof(heap_header_t);
				// Calculate leftover block used size
				uint64 left_size = block_offset - block_addr;
				// Calculate used size of the aligned block
				uint64 right_size = block_size - left_size;
                if (left_size >= HEAP_LIST_MIN && right_size >= bsize){
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
    // We didn't find anytghing
	return 0;
}
/**
* Search for a free block that matches size criteria
* @param tree - a pointer to a root of the search tree
* @param bsize - block size
* @return a pointer to heap block header
*/
static heap_header_t *heap_tree_search(free_node_t **tree, uint64 bsize){
    free_node_t *free_block = (*tree);
    // Return - there's nothing to search into
    if (free_block == 0){
        return 0;
    }
    // Move our search pointer to the first suitable entry
    // First we test for a larger block if current one is too small
    while (HEAP_GET_SIZE(free_block) < bsize && free_block->larger_block != 0){
		free_block = free_block->larger_block;
	}
    // Return - the block is too small and there are no more larger blocks left
    if (HEAP_GET_SIZE(free_block) < bsize){
        return 0;
    }
    // Then we try to find a smaller one that will not waste too much space
	while (HEAP_GET_SIZE(free_block) > bsize && free_block->smaller_block != 0 && HEAP_GET_SIZE(free_block->smaller_block) >= bsize){
		free_block = free_block->smaller_block;
	}
    // We might have found some thing
	return (heap_header_t *)free_block;
}
/**
* Create a heap block. Write header and footer information.
* @param ptr - free memory location on which to build a heap block
* @param psize - payload size
*/
static void heap_create_block(void *ptr, uint64 bsize){
	// Cast this void space to a header of the heap block 
	heap_header_t *header = (heap_header_t *)ptr;

	// Set header
	header->magic = HEAP_MAGIC;
	header->size = bsize;
	
	// Move to footer
	heap_footer_t *footer = HEAP_GET_FOOTER(header);

	// Set footer
	footer->header = header;
	footer->magic = HEAP_MAGIC;
}
/**
* Split a block at a spcific location (relative to payload size)
* @param block - block to split
* @param offset - split offset
* @return newly created block to the right
*/
static heap_header_t *heap_split_block(heap_header_t *block, uint64 offset){
    // Caclutate leftover payload size
	uint64 block_size = HEAP_GET_SIZE(block);
    // Calculate the new payload and block sizes of the block
    offset = HEAP_ALIGN(offset);
    // Calculate payload and block sizes of the leftover block
    uint64 leftover_size = block_size - offset;
    if (leftover_size >= HEAP_LIST_MIN){
		// Split if the leftover is large enough
		heap_header_t *leftover = (heap_header_t *)(((uint64)block) + offset);
        // Create a new block
		heap_create_block(leftover, leftover_size);
		// Recreate the old block
		heap_create_block(block, offset);
		return leftover;
	}
	return 0;
}
/**
* Search for an apropriate block in the free list or a tree
* @param heap - pointer to the heap
* @param bsize - block size
* @param align - align the block to the page boundary
* @return a free block
*/
static heap_header_t *heap_search(heap_t *heap, uint64 bsize, bool align){
    // Find the block in the list
	int8 list_idx = HEAP_SIZE_IDX(bsize);
    if (list_idx >= 0){
		while (list_idx < HEAP_LIST_COUNT){
			if (heap->free[list_idx] != 0){
				free_item_t *free_block = (free_item_t *)heap->free[list_idx];
				if (!align || HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(free_block))){
					// No alignament required or this block is just perfect
					return (heap_header_t *)free_block;
				} else {
					// Try to find one that's well aligned in the same list
					while (free_block->next_block != 0){
						if (HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(free_block))){
							// This one is perfect
							return (heap_header_t *)free_block;
						} else {
							// Test if we can split it at the boundary
							uint64 block_addr = (uint64)free_block;
							uint64 block_size = HEAP_GET_SIZE(free_block);
							uint64 payload_addr = (uint64)HEAP_GET_PAYLOAD(free_block);
							// Calculate aligned payload address
							uint64 payload_offset = PAGE_SIZE_ALIGN(payload_addr);
							// Calculate the block address after alignament
							uint64 block_offset = payload_offset - sizeof(heap_header_t);
							// Calculate leftover block used size
							uint64 left_size = block_offset - block_addr;
							// Calculate used size of the aligned block
							uint64 right_size = block_size - left_size;
                            if (left_size >= HEAP_LIST_MIN && right_size >= bsize){
								// Everything seems fine:
								// * leftover can be split of
								// * the block will suite fine after alignament
								return (heap_header_t *)free_block;
							}
						}
						// Move to the next one in the same list
						free_block = free_block->next_block;
					}
				}
			}
			// Move to the next list
			list_idx ++;
		}
	}
	// Didn't find it - look up in the tree
    if (align){
        // Search for aligned block
        return heap_tree_search_aligned((free_node_t **)&heap->free[HEAP_LIST_COUNT], bsize);
    } else {
        // Search for a block that just fits
        return heap_tree_search((free_node_t **)&heap->free[HEAP_LIST_COUNT], bsize);
    }
}
/**
* Remove a block form the free list or a tree
* @param heap - pointer to the heap
* @param free_block - block to remove
*/
static void heap_remove(heap_t *heap, heap_header_t *block){
	// Try to delete from the list
	uint64 bsize = HEAP_GET_SIZE(block);
	int8 list_idx = HEAP_SIZE_IDX(bsize);
    free_item_t *free_block = (free_item_t *)block;
	if (list_idx >= 0){
		if (free_block->prev_block == 0){
			if (free_block->next_block == 0){
				// This was the only block on the lists
				heap->free[list_idx] = 0;
			} else {
				// This is the top block
				free_block->next_block->prev_block = 0;
				heap->free[list_idx] = (heap_header_t *)free_block->next_block;
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
		// Clear pointers
		free_block->next_block = 0;
		free_block->prev_block = 0;
	} else {
		// List does not accept it - delete it from the tree
		heap_tree_delete((free_node_t **)(&(heap->free[HEAP_LIST_COUNT])), block);
	}
}
/**
* Insert a block into the free list or a tree
* @param heap - pointer to the heap
* @param free_block - block to insert
*/
static void heap_insert(heap_t *heap, heap_header_t *block){
	// Try to push the block on the list
	uint64 bsize = HEAP_GET_SIZE(block);
    int8 list_idx = HEAP_SIZE_IDX(bsize);
    free_item_t *free_block = (free_item_t *)block;
	if (list_idx >= 0){
		// Add to the list
		free_block->prev_block = 0;
		free_block->next_block = (free_item_t *)heap->free[list_idx];
		if (free_block->next_block != 0){
			free_block->next_block->prev_block = free_block;
		}
		heap->free[list_idx] = (heap_header_t *)free_block;
	} else {
		// List does not accept it - insert it into the tree
		heap_tree_insert((free_node_t **)(&(heap->free[HEAP_LIST_COUNT])), block);
	}
}
/**
* Merge block to the left
* @param free_block - the block you try to merge
* @return same block or the left one merged with the one you passed
*/
static heap_header_t * heap_merge_left(heap_t *heap, heap_header_t *block){
	if (((uint64)block) > heap->start_addr){
        heap_footer_t *prev_footer = HEAP_LEFT_FOOTER(block);
        if (HEAP_CHECK_FOOTER(prev_footer) &&  HEAP_GET_USED(HEAP_GET_HEADER(prev_footer)) == 0){
			// Merge left
			heap_header_t *free_left = (heap_header_t *)HEAP_GET_HEADER(prev_footer);
			// Remove the right block
			heap_remove(heap, free_left);
			// Create new block
			heap_create_block(free_left, HEAP_GET_SIZE(free_left) + HEAP_GET_SIZE(block));
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
static heap_header_t * heap_merge_right(heap_t *heap, heap_header_t *block){
	if (((uint64)HEAP_GET_FOOTER(block)) + sizeof(heap_footer_t) < heap->end_addr){
        heap_header_t *next_header = HEAP_RIGHT_HEADER(block);
		if (HEAP_CHECK_HEADER(next_header) && HEAP_GET_USED(next_header) == 0){
			// Merge left
			heap_header_t *free_right = (heap_header_t *)next_header;
			// Remove the right block
			heap_remove(heap, free_right);
			// Create new block
			heap_create_block(block, HEAP_GET_SIZE(block) + HEAP_GET_SIZE(free_right));
		}
	}
    return block;
}
/**
* Extend heap size within a page size aligned size
* @param heap - pointer to the beginning of the heap
* @param size - required payload size
* @return a new free block
*/
static heap_header_t * heap_extend(heap_t *heap, uint64 bsize){
	heap_header_t *block = (heap_header_t *)(heap->end_addr);
	uint64 alloc_size = page_alloc(heap->end_addr, bsize);
    if (alloc_size > 0){
		heap->end_addr += alloc_size;
	    // Create a new free block
	    heap_create_block(block, alloc_size);
	    // Try to merge with the last one
	    return heap_merge_left(heap, block);
    }
	return 0;
}

heap_t * heap_create(uint64 start, uint64 size, uint64 max_size){
	heap_t *heap = (heap_t *)start;
	start += sizeof(heap_t);
    size -= sizeof(heap_t);
    max_size -= sizeof(heap_t);
	
	heap->start_addr = start;
	heap->end_addr = start + size;
	heap->max_addr = start + max_size;
    heap->flags.locked = 0;
    heap->flags.reserved = 0;

	// Initialize free lists
	uint8 i = 0;
	for (i = 0; i <= HEAP_LIST_COUNT; i ++){
		heap->free[i] = 0;
	}

	// Initialize free tree
	heap_header_t *block = (heap_header_t *)(heap->start_addr);

	// Initialize first block in the free tree
    heap_create_block(block, size);
    
    // Insert the block into the heap
    heap_insert(heap, block);

	return heap;
}

void *heap_alloc(heap_t * heap, uint64 size, bool align){
    heap_wait_lock(heap);
    heap_header_t *free_block = 0;

    if (size > 0){
        // Align the requested payload size to heap alignament
        uint64 psize = HEAP_SIZE_ALIGN(size);
        uint64 bsize = HEAP_BSIZE(psize);
	
        // Search for a free block
	    free_block = heap_search(heap, bsize, align);

        if (free_block == 0){
		    // Allocate a new page if all the free space has been 
            // This one comes removed from the tree already
		    free_block = heap_extend(heap, bsize);
	    } else {
            // Remove block
		    heap_remove(heap, free_block);
	    }
	    if (free_block != 0){
		    if (align && !HEAP_IS_PAGE_ALIGNED(HEAP_GET_PAYLOAD(free_block))){
                // We need to create an aligned block first
			    uint64 block_addr = (uint64)free_block;
			    uint64 block_size = HEAP_GET_SIZE(free_block);
			    uint64 payload_addr = (uint64)HEAP_GET_PAYLOAD(free_block);
			    // Calculate aligned payload address
			    uint64 payload_offset = PAGE_SIZE_ALIGN(payload_addr);
			    // Calculate the block address after alignament
			    uint64 block_offset = payload_offset - sizeof(heap_header_t);
			    // Calculate leftover block used size
			    uint64 left_size = block_offset - block_addr;
			    // Calculate used size of the aligned block
			    uint64 right_size = block_size - left_size;
			    bsize = HEAP_BSIZE(psize);
			    // Create leftover block
			    heap_create_block((void *)free_block, left_size);
			    // Create the real block
			    heap_create_block((void *)block_offset, right_size);
			    // Push the leftover back to list or a tree
			    heap_insert(heap, free_block);
			
			    free_block = (heap_header_t *)block_offset;
		    }
		
		    // Try to split this block
		    heap_header_t * free_other = heap_split_block(free_block, bsize);
		    if (free_other != 0){
                // If there are left overs - add them to list or a tree
			    heap_insert(heap, free_other);
		    }

		    // Mark used in header
		    HEAP_SET_USED(free_block, 1);
		    // Return a pointer to the payload
            heap_unlock(heap);
            return (void *)HEAP_GET_PAYLOAD(free_block);
	    }
    }
    heap_unlock(heap);
	return 0;
}

void *heap_realloc(heap_t * heap, void *ptr, uint64 size, bool align){
	uint64 psize = HEAP_ALIGN(size);
    uint64 bsize = HEAP_BSIZE(psize);
    uint64 psize_now = heap_alloc_size(ptr);
	if (psize_now > psize){
		// For now, but we should check if it can be splitted and some parts freed
		return ptr;
	} else {
		// This is easy :)
		heap_wait_lock(heap);
        void *ptr_new = heap_alloc(heap, bsize, align);
		mem_copy((uint8 *)ptr_new, psize_now, (uint8 *)ptr);
		heap_free(heap, ptr);
        heap_unlock(heap);
		return ptr_new;
	}
}

void heap_free(heap_t * heap, void *ptr){
	if (ptr != 0){
		heap_header_t *free_block = (heap_header_t *)HEAP_PAYLOAD_HEADER(ptr);
		if (HEAP_CHECK_HEADER(free_block)){
            heap_wait_lock(heap);
			// Clear used in header
			HEAP_SET_USED(free_block, 0);

			// Try to merge on the left side
			free_block = heap_merge_left(heap, free_block);
		
			// Try to merge on the right side
			heap_merge_right(heap, free_block);

			// Add this block back to list or tree
			heap_insert(heap, free_block);
            heap_unlock(heap);
		}
	}
}

uint64 heap_alloc_size(void *ptr){
	if (ptr != 0){
		heap_header_t *free_block = (heap_header_t *)HEAP_PAYLOAD_HEADER(ptr);
		if (HEAP_CHECK_HEADER(free_block)){
			return HEAP_PSIZE(HEAP_GET_SIZE(free_block));
		}
	}
	return 0;
}

#if DEBUG == 1
void heap_list(heap_t *heap){
	int8 i = 0;
	uint64 count = 0;

	// Print out the heap
	heap_header_t *block = (heap_header_t *)(heap->start_addr + sizeof(heap_t));
	debug_print(DC_WB, "Heap start @%x end %x", (uint64)block, heap->end_addr);
	while (HEAP_CHECK_HEADER(block)){
		debug_print(DC_WB, "    Block @%x (size: %d, used: %d)", (uint64)block, HEAP_GET_SIZE(block), HEAP_GET_USED(block));
		block = HEAP_RIGHT_HEADER(block);
	}

	// Print the free lists
	free_item_t *item;
	for (i = 0; i < HEAP_LIST_COUNT; i ++){
		if (heap->free[i] != 0){
			debug_print(DC_WBL, "List %d", (int64)i);
			item = (free_item_t *)heap->free[i];
			count = 1;
			while (item->next_block != 0){
				count ++;
				item = item->next_block;
			}
			debug_print(DC_WDG, "    root item @%x", (uint64)heap->free[i]);
			debug_print(DC_WDG, "    total items %d", count);
		}
	}
	
	// Print the free tree
	if (heap->free[HEAP_LIST_COUNT] != 0){
		free_node_t *tree = (free_node_t *)heap->free[HEAP_LIST_COUNT];
		debug_print(DC_WBL, "Tree root @%x size %d", (uint64)tree, (uint64)HEAP_GET_SIZE(tree));
		free_node_t *node = tree;
		while (node->smaller_block != 0){
			node = node->smaller_block;
			debug_print(DC_WDG, "    s-node @%x size %d", (uint64)node, node->header.size);
		}
		node = tree;
		while (node->larger_block != 0){
			node = node->larger_block;
			debug_print(DC_WDG, "    l-node @%x size %d", (uint64)node, node->header.size);
		}
	}
}
#endif
