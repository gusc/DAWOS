/*

Memory paging functions
=======================

License (BSD-3)
===============

Copyright (c) 2012, Gusts 'gusC' Kaksis <gusts.kaksis@gmail.com>
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
#include "memory.h"
#include "lib.h"
#include "cr.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

/**
* Memory type codes for E820
*/
enum eMemType {
	kMemOk = 1,			// Normal memory - usable
	kMemReserved,		// Reserved memory - unusable
	kMemACPIReclaim,	// ACPI reclaimable memory - might be usable after ACPI is taken care of
	kMemACPI,			// ACPI NVS memory - unusable
	kMemBad				// Bad memory - unsuable
};
static char *types[] = {
  "Free",
  "Reserved",
  "ACPI Reclaimable",
  "ACPI NVS",
  "Bad"
};
/**
* E820 memory map entry structure
*/
struct e820entry_struct {
	uint16 entry_size;	// if 24, then it has attributes
	uint64 base;
	uint64 length;
	uint32 type;
	uint32 attributes;	// ACPI 3.0 only
} __PACKED;
typedef struct e820entry_struct e820entry_t;
/**
* E820 memory map structure
*/
struct e820map_struct {
	uint16 size;
	e820entry_t entries[];
} __PACKED;
typedef struct e820map_struct e820map_t;

/**
* Page table entry structure
*/
typedef struct {
	uint64 present			: 1;	// Is the page present in memory?
	uint64 writable			: 1;	// Is the page writable?
	uint64 user				: 1;	// Is the page for userspace?
	uint64 write_through	: 1;	// Do we want write-trough? (when cached, this also writes to memory)
	uint64 cache_disable	: 1;	// Disable cache on this page?
	uint64 accessed			: 1;	// Has the page been accessed by software?
	uint64 dirty			: 1;	// Has the page been written to since last refresh? (ignored in PML4E, PML3E, PML2E)
	uint64 pat				: 1;	// Page attribute table (in PML1E), 
									// page size bit (must be 0 in PML4E, in PML3E 1 = 1GB page size, in PML2E 1 = 2MB page size otherwise 4KB pages are used)
	uint64 global			: 1;	// Is the page global? (ignored in PML4E, PML3E, PML2E)
	uint64 data				: 3;	// Ignored (ignored in all PML levels)
	uint64 frame			: 40;	// Frame address (4KB aligned)
	uint64 data2			: 11;	// Ignored (ignored in all PML levels)
	uint64 xd				: 1;	// Execute disable bit (whole region is not accessible by instruction fetch)
} pm_t;
/**
* Page block header structure
*/
struct page_header_struct {
	uint64 magic;
	uint64 size;
} __PACKED;
typedef struct page_header_struct page_header_t;
/**
* Heap block footer structure
*/
struct page_footer_struct {
	uint64 magic;
	page_header_t *header;
} __PACKED;
typedef struct page_footer_struct page_footer_t;
/**
* Free block structure for free lists - header + pointers to next and previous blocks
*/
typedef struct free_item_struct free_item_t;
struct free_item_struct {
	page_header_t header;
	free_item_t *prev_block; // Previous block in the segregated list
	free_item_t *next_block; // Next block in the segregated list
} __PACKED;
/**
* Free block structure for search tree - header + pointers to next and previous blocks
*/
typedef struct free_node_struct free_node_t;
struct free_node_struct {
	page_header_t header;
	free_node_t *smaller_block; // Smaller block in the sorted list
	free_node_t *larger_block; // Larger block in the sorted list
	free_node_t *child_block; // Equaly sized child block
	free_node_t *parent_block; // Parent block of equaly sized child
} __PACKED;

// Magic number used in heap blocks for sanity checks
#define PAGE_MAGIC 0xFFFFDEADBEEFFFFF
// Segeregate list min size
#define PAGE_LIST_MIN 1
// Segeregate list max size
#define PAGE_LIST_MAX 32
// Segregated list count
#define PAGE_LIST_COUNT (PAGE_LIST_MAX - PAGE_LIST_MIN)
// Make virtual address canonical (sign extend)
#define PAGE_CANONICAL(va) ((va << 16) >> 16)
// Page table entry index mask
#define PAGE_PML_IDX_MASK 0x1FF
// Page offset mask
#if PAGE_LEVELS == 2
	#define PAGE_OFFSET_MASK   0x3FFFFF
#elif PAGE_LEVELS == 3
	#define PAGE_OFFSET_MASK   0x1FFFFF
#else
	#define PAGE_OFFSET_MASK   0xFFF
#endif
// Page frame mask (40bits shifter 12bits left)
#define PAGE_FRAME_MASK 0xFFFFFFFFFF000
/**
* Get table entry index from virtual address
* @param va - virtual address
* @param lvl - PML level
* @return idx - index of the entry
*/
#define PAGE_PML_IDX(va, lvl) ((va >> (12 + ((lvl - 1) * 9))) & PAGE_PML_IDX_MASK)
/**
* Get the physical address from page table at index
* @param pt - page table
* @param idx - index of the entry
* @return paddr - physical address
*/
#define PAGE_ADDRESS(pt, idx) (((uint64 *)pt)[idx] & PAGE_FRAME_MASK)
/**
* Get page frame number from physical address (52bit)
* @param paddr - physical address
* @return frame - frame number
*/
#define PAGE_FRAME(paddr) ((paddr & PAGE_FRAME_MASK) >> 12)
/**
* Get a pointer to the footer from header
* @param h - pointer to the header
* @return pointer to the footer
*/
#define PAGE_GET_FOOTER(h) ((page_footer_t *)(((uint8 *)h) + ((page_header_t *)h)->size - sizeof(page_footer_t)))
/**
* Get a pointer to the header from footer
* @param f - pointer to the footer
* @return pointer to the header
*/
#define PAGE_GET_HEADER(f) (((page_footer_t *)f)->header)
/**
* Do the sanity check on header
* @param h - pointer to the header
* @return bool
*/
#define PAGE_CHECK_HEADER(h) (((page_footer_t *)h)->magic == PAGE_MAGIC && PAGE_GET_FOOTER(h)->magic == PAGE_MAGIC)
/**
* Do the sanity check on footer
* @param f - pointer to the footer
* @return bool
*/
#define PAGE_CHECK_FOOTER(f) (((page_footer_t *)f)->magic == PAGE_MAGIC && ((page_footer_t *)f)->header->magic == PAGE_MAGIC)
/**
* Calculate list index from the size
* @param s - block size
* @return idx - list index
*/
#define PAGE_SIZE_IDX(s) ((s / PAGE_SIZE) <= PAGE_LIST_MAX ? (s / PAGE_SIZE) : -1);

/**
* Page table structures
*/
static pm_t *_pml4;

static uint64 _total_mem = 0;
static uint64 _available_mem = 0;

static free_item_t *_page_list[PAGE_LIST_COUNT];
static free_node_t *_page_tree;
/**
* Placeholder for the next virtual address
*/
static uint64 page_placeholder = INIT_MEM;

/**
* Parse memory map and sort in ascending order
* Also determine maximum size and available size
* @param mem_map - a pointer to E820 location in memory
*/
static void sort_e820(e820map_t *mem_map){
	uint64 i = 0;
	// Do the bubble sort to make them in ascending order
	e820entry_t e;
	uint64 swapped  = 1;
	uint64 count = mem_map->size;
	while (count > 0 && swapped){
		i = 0;
		swapped  = 0;
		while (i < count - 1){
			if (mem_map->entries[i].base > mem_map->entries[i + 1].base){
				e = mem_map->entries[i];
				mem_map->entries[i] = mem_map->entries[i + 1];
				mem_map->entries[i + 1] = e;
				swapped  = 1;
			}
			i ++;
		}
		count --;
	}
}
/**
* Create a heap block. Write header and footer information.
* @param ptr - free memory location on which to build a page block
* @param size - block size (including header and footer)
*/
static void page_create_block(void *ptr, uint64 size){
	uint64 addr = PAGE_ALIGN((uint64)ptr);
	size = PAGE_SIZE_ALIGN(size);

	// Cast this void space to a header of the heap block 
	page_header_t *header = (page_header_t *)ptr;

	// Set header
	header->magic = PAGE_MAGIC;
	header->size = size;
	
	// Move to footer
	page_footer_t *footer = PAGE_GET_FOOTER(header);

	// Set footer
	footer->header = header;
	footer->magic = PAGE_MAGIC;
}
/**
* Insert a free block into a sorted list
* @param block - block to add
* @return true on success, false if the block was too small for tree structure
*/
static bool page_tree_insert(page_header_t *block){
	if (block->size / PAGE_SIZE > PAGE_LIST_MAX){
		free_node_t *free_block = (free_node_t *)block;
		if (_page_tree == 0){
			// As easy as it can be
			_page_tree = free_block;
			_page_tree->parent_block = 0;
			_page_tree->child_block = 0;
			_page_tree->smaller_block = 0;
			_page_tree->larger_block = 0;
			return true;
		} else {
			uint64 usize = free_block->header.size;
			free_node_t *block = _page_tree;
			free_node_t *parent_prev = 0;
			uint64 block_usize;
			bool set = false;
			while (!set){
				block_usize = block->header.size;
				if (block_usize > usize){
					// Smaller than selected
					if (block->smaller_block == 0){
						// No more where to go - store here
						block->smaller_block = free_block;
						free_block->larger_block = block;
						free_block->parent_block = 0;
						free_block->child_block = 0;
						return true;
					} else if (block->smaller_block->header.size < usize){
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
					} else if (block->larger_block->header.size > usize){
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
* @param block - block to remove
*/
static void page_tree_delete(page_header_t *block){
	free_node_t *free_block = (free_node_t *)block;
	if (free_block->parent_block != 0) {
		// This is a child item
		// Simply remove this block from it's parent
		// There should never be any larger or smaller blocks for childs
		free_block->parent_block->child_block = free_block->child_block;
	} else {
		free_node_t *replacement = 0;
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
		if (_page_tree == free_block){
			// Replace the root with something
			_page_tree = replacement;
		}
	}
}
/**
* Search for a free block that matches size and alignament criteria
* @param size - block size
* @return a pointer to page block header
*/
static page_header_t *page_tree_search(uint64 size){
	free_node_t *free_block = _page_tree;

	// Move our search pointer to the first suitable entry
	if (free_block->header.size < size){
		while (free_block != 0 && free_block->header.size < size){
			free_block = free_block->larger_block;
		}
	} else if (free_block->header.size > size) {
		while (free_block->header.size > size){
			if (free_block->smaller_block != 0 && free_block->smaller_block->header.size >= size){
				free_block = free_block->smaller_block;
			} else {
				break;
			}
		}
	}
	// We might have found something for an unaligned request (it might be 0 too)
	return (page_header_t *)free_block;
}

/**
* Push a free block to the segregated list
* @param block - block to add
* @return true on success, false if the block was too large for free lists
*/
static bool page_list_push(page_header_t *block){
	int8 list_idx = PAGE_SIZE_IDX(block->size);
	free_item_t *free_block = (free_item_t *)block;
	if (list_idx >= 0){
		// Add to the list
		free_block->prev_block = 0;
		free_block->next_block = _page_list[list_idx];
		if (free_block->next_block != 0){
			free_block->next_block->prev_block = free_block;
		}
		_page_list[list_idx] = free_block;
		return true;
	}
	return false;
}
/**
* Delete a free block from the segregated list
* @param bloc - block to remove
* @return true on success
*/
static bool page_list_delete(page_header_t *block){
	int8 list_idx = PAGE_SIZE_IDX(block->size);
	free_item_t *free_block = (free_item_t *)block;
	if (list_idx >= 0){
		if (free_block->prev_block == 0){
			if (free_block->next_block == 0){
				// This was the only block on the lists
				_page_list[list_idx] = 0;
			} else {
				// This is the top block
				_page_list[list_idx] = free_block->next_block;
				_page_list[list_idx]->prev_block = 0;
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
* @param size - block size
* @return a free block
*/
static page_header_t *page_list_search(uint64 size){
	int8 list_idx = PAGE_SIZE_IDX(size);
	if (list_idx >= 0){
		while (list_idx < PAGE_LIST_COUNT){
			// Check it list contains any items
			if (_page_list[list_idx] != 0){
				return (page_header_t *)_page_list[list_idx];
			}
			// Move to the next list
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
static page_header_t * page_merge_left(page_header_t *block){
	if (((uint64)block) >= PAGE_SIZE){
		page_footer_t *prev_footer = (page_footer_t *)(((uint8 *)block) - sizeof(page_footer_t));
		if (PAGE_CHECK_FOOTER(prev_footer)){
			// Merge left
			page_header_t *free_left = (page_header_t *)PAGE_GET_HEADER(prev_footer);
			// Remove the right block
			if (!page_list_delete(free_left)){
				page_tree_delete(free_left);
			}
			// Create new block
			page_create_block(free_left, free_left->size + block->size);
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
static void page_merge_right(page_header_t *block){
	page_footer_t *footer = (page_footer_t *)PAGE_GET_FOOTER(block);
	if (((uint64)footer) + sizeof(page_footer_t) < _total_mem){
		page_header_t *next_header = (page_header_t *)(((uint8 *)footer) + sizeof(page_footer_t));
		if (PAGE_CHECK_HEADER(next_header)){
			// Merge left
			page_header_t *free_right = (page_header_t *)next_header;
			// Remove the right block
			if (!page_list_delete(free_right)){
				page_tree_delete(free_right);
			}
			// Create new block
			page_create_block(block, block->size + free_right->size);
		}
	}
}

void page_init(uint64 ammount){
	_pml4 = (pm_t *)get_cr3();
	// Read E820 memory map and mark used regions
	e820map_t *mem_map = (e820map_t *)E820_LOC;
	// Sort memory map
	sort_e820(mem_map);
	
	uint64 i;
	uint64 k;
	uint64 va;
	
	// Clear page free lists and tree
	_page_tree = 0;
	for (i = 0; i < PAGE_LIST_COUNT; i ++){
		_page_list[i] = 0;
	}
	
	// Get total RAM and map some pages
	for (i = 0; i < mem_map->size; i ++){
		if (mem_map->entries[i].base + mem_map->entries[i].length > _total_mem){
			_total_mem = mem_map->entries[i].base + mem_map->entries[i].length;
		}
		for (k = 0; k < mem_map->entries[i].length; k += PAGE_SIZE){
			va = mem_map->entries[i].base + k;
			page_map(va, va);
		}
		if (mem_map->entries[i].type == kMemOk){
			_available_mem += mem_map->entries[i].length;
			if (mem_map->entries[i].base + mem_map->entries[i].length > INIT_MEM){
				// Mark rest of the memory free
				uint64 start = mem_map->entries[i].base;
				if (start < INIT_MEM){
					start = INIT_MEM;
				}
				uint64 size = mem_map->entries[i].length - (start - mem_map->entries[i].base);
				page_create_block((void *)start, size);
				if (!page_list_push((page_header_t *)start)){
					page_tree_insert((page_header_t *)start);
				}
			}
		}
#if DEBUG == 1
		debug_print(DC_WB, "%x - %x (%s)", mem_map->entries[i].base, mem_map->entries[i].base + mem_map->entries[i].length, types[mem_map->entries[i].type - 1]);
#endif
	}

	// Register page fault handler
	interrupt_reg_handler(14, page_fault);
}
uint64 page_fault(int_stack_t *stack){
	uint64 fail_addr = get_cr2();
#if DEBUG == 1
	// From JamesM tutorials
	int present   = !(stack->err_code & 0x1); // Page not present
	int rw = stack->err_code & 0x2;           // Write operation?
	int us = stack->err_code & 0x4;           // Processor was in user-mode?
	int reserved = stack->err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
	int id = stack->err_code & 0x10;          // Caused by an instruction fetch?

	debug_print(DC_WRD, "Page fault");
	if (present) {debug_print(DC_WRD, "present ");}
	if (rw) {debug_print(DC_WRD, "read-only ");}
	if (us) {debug_print(DC_WRD, "user-mode ");}
	if (reserved) {debug_print(DC_WRD, "reserved ");}
	debug_print(DC_WRD, "Error: %x", stack->err_code);
	debug_print(DC_WRD, "Addr: @%x", fail_addr);
#endif

	HANG();
			

	// Map this page if it's not mapped yet, otherwise hang
	if (!page_resolve(fail_addr)){
		if (!page_map(fail_addr, fail_addr)){
			HANG();
			return 1;
		}
	} else {
		HANG();
		return 1;
	}
	return 0;
}
uint64 page_total_mem(){
	return _total_mem;
}
uint64 page_available_mem(){
	return _available_mem;
}
bool page_id_map(uint64 paddr, uint64 vaddr, bool mmio){
	// Do the identity map
	pm_t *table = (pm_t *)page_get_pml4();
	pm_t *ct;
	uint8 i;
	uint64 idx;
	vaddr = PAGE_CANONICAL(vaddr);
	for (i = 4; i > 1; i --){
		// Get page table entry index from the virtual address
		idx = PAGE_PML_IDX(vaddr, i);
		if (!table[idx].present){
			// Next level table does not exist - create one
			ct = (pm_t *)mem_alloc_align(sizeof(pm_t) * 512);
			mem_fill((uint8 *)ct, sizeof(pm_t) * 512, 0);

			// Store the physical address
			table[idx].frame = PAGE_FRAME((uint64)ct);
			// Set the flags
			table[idx].present = 1;
			table[idx].writable = 1;
			table[idx].write_through = 1;
			if (mmio){
				table[idx].cache_disable = 1;
			}
			// Continiue with the newly created table
			table = ct;
		} else {
			// Find next level table address
			table = (pm_t *)PAGE_ADDRESS(table, idx);
			//debug_print(DC_WB, "Entry: @%x", (uint64)table);
		}
	}
	// Get page index in PML1 table from virtual address 
	idx = PAGE_PML_IDX(vaddr, 1);
	// Last level table
	if (!table[idx].present){
		// Store physical address
		table[idx].frame = PAGE_FRAME(paddr);
		// Set the flags
		table[idx].present = 1;
		table[idx].writable = 1;
		table[idx].write_through = 1;
		if (mmio){
			table[idx].cache_disable = 1;
		}
		return true;
	}
	return false;
}
bool page_release(uint64 vaddr){
	//TODO: release
	return true;
}
bool page_map(uint64 paddr, uint64 vaddr){
	return page_id_map(paddr, vaddr, false);
}
bool page_map_mmio(uint64 paddr, uint64 vaddr){
	return page_id_map(paddr, vaddr, true);
}
uint64 page_resolve(uint64 vaddr){
	pm_t *table = (pm_t *)page_get_pml4();
	uint8 i = 4;
	uint64 idx = PAGE_PML_IDX(vaddr, i);
	for (i = 3; i >= 1; i --){
		if (table[idx].present){
			// Next level table exists - load it's address
			table = (pm_t *)PAGE_ADDRESS(table, idx);
			// Get next level index from this table
			idx = PAGE_PML_IDX(vaddr, i);
		} else {
			return 0;
		}
	}
	return PAGE_ADDRESS(table, idx) + (vaddr & PAGE_OFFSET_MASK);
}

uint64 page_alloc(uint64 vaddr, uint64 size){
	size = PAGE_SIZE_ALIGN(size);

	if (!page_resolve(vaddr)){
		if (!page_map(vaddr, vaddr)){
			return 0;	
		}
	}

	page_header_t *pages = page_list_search(size);
	if (pages == 0){
		pages = page_tree_search(size);
	}

	if (pages->size > size){
		//page_split
	}

	return size;
}
void page_free(uint64 vaddr, uint64 size){
	/*pm_t *table = (pm_t *)page_get_pml4();
	uint8 i = 4;
	uint64 idx = PAGE_PML_IDX(vaddr, i);
	for (i = 3; i >= 1; i --){
		if (table[idx].present){
			// Next level table exists - load it's address
			table = (pm_t *)PAGE_ADDRESS(table, idx);
			// Get next level index from this table
			idx = PAGE_PML_IDX(vaddr, i);
		} else {
			return;
		}
	}
	table[idx].present = 0;
	uint64 paddr = PAGE_ADDRESS(table, idx);*/
	// TODO: check if some page table is empty and delete it's parent
}

uint64 page_get_pml4(){
	return get_cr3();
}

void page_set_pml4(uint64 paddr){
	set_cr3(paddr);
}