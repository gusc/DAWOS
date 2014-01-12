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
* Page table structures
*/
static pm_t *_pml4;

static uint64 _total_mem = 0;
static uint64 _available_mem = 0;

static uint64 *_page_frames;
static uint64 _page_count = 0;

/**
* Placeholder for the next virtual address
*/
static uint64 page_placeholder = INIT_MEM;

// Virtual address parts masks
//#define PAGE_PML4_IDX_MASK 0xFF8000000000
//#define PAGE_PML3_IDX_MASK 0x7FC0000000
//#define PAGE_PML2_IDX_MASK 0x3FE00000
#define PAGE_PML_IDX_MASK 0x1FF
#if PAGE_LEVELS == 2
	#define PAGE_OFFSET_MASK   0x3FFFFF
#elif PAGE_LEVELS == 3
	#define PAGE_OFFSET_MASK   0x1FFFFF
#else
	#define PAGE_OFFSET_MASK   0xFFF
#endif
/**
* Get table entry index from virtual address
* @param va - virtual address
* @param lvl - PML level
* @return idx - index of the entry
*/
#define PAGE_PML_IDX(va, lvl) ((va >> (12 + ((lvl - 1) * 9))) & PAGE_PML_IDX_MASK)
/**
* Get bitset index
* @param p - page number
* @return index
*/
#define PAGE_FRAME_INDEX(p) (p / 64)
/**
* Get bit offset in bitset
* @param p - page number
* @return offset
*/
#define PAGE_FRAME_BIT(p) (p % 64)
/**
* Get the physical address from page table at index
* @param pt - page table
* @param idx - index of the entry
* @return paddr - physical address
*/
#define PAGE_ADDRESS(pt, idx) (((pm_t *)pt)[idx].raw & PAGE_MASK)

/**
* Mark frame allocated
* @param paddr - physical address to mark
*/
static void page_set_frame(uint64 paddr){
    uint64 page = paddr / 4069;
    uint64 idx = PAGE_FRAME_INDEX(page);
    uint64 offset = PAGE_FRAME_BIT(page);
	_page_frames[idx] |= (0x1 << offset);
}
/**
* Mark frame free
* @param paddr - physical address to mark
*/
static void page_clear_frame(uint64 paddr){
    uint64 page = paddr / 4069;
    uint64 idx = PAGE_FRAME_INDEX(page);
    uint64 offset = PAGE_FRAME_BIT(page);
	_page_frames[idx] &= ~(0x1 << offset);
}
/**
* Check if frame is allocated or free
* @param paddr - physical address to check
* @return true if set false if not
*/
static bool page_check_frame(uint64 paddr){
    uint64 page = paddr / 4069;
    uint64 idx = PAGE_FRAME_INDEX(page);
    uint64 offset = PAGE_FRAME_BIT(page);
    return (_page_frames[idx] & (0x1 << offset));
}

/**
* Parse memory map and sort in ascending order
* Also determine maximum size and available size
*/
static void parse_e820(e820map_t *mem_map){
	uint64 i = 0;
	// Do the bubble sort to make them in ascending order
	e820entry_t e;
	uint64 swapped  = 1;
	uint64 count = mem_map->size;
	while (count > 0 && swapped ){
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
	// Get total RAM
	for (i = 0; i < mem_map->size; i ++){
		if (mem_map->entries[i].type != kMemReserved){
			if (mem_map->entries[i].base + mem_map->entries[i].length > _total_mem){
				_total_mem = mem_map->entries[i].base + mem_map->entries[i].length;
			}
			if (mem_map->entries[i].type == kMemOk){
				_available_mem += mem_map->entries[i].length;
			}
		}
	}
}

void page_init(uint64 ammount){
	_pml4 = (pm_t *)get_cr3();
	// Read E820 memory map and mark used regions
	e820map_t *mem_map = (e820map_t *)E820_LOC;
	// Parse memory map
	parse_e820(mem_map);
		
	// Calculate total frame count
	_page_count = _total_mem / PAGE_SIZE;
#if DEBUG == 1
	debug_print(DC_WB, "Frames: %d", _page_count);
#endif

	// Identity map rest of the memory
	uint64 i = _page_count - (INIT_MEM / PAGE_SIZE);
	for (; i < _page_count; i ++){
		page_map(i * PAGE_SIZE, i * PAGE_SIZE);
	}

	// Allocate frame bitset at the next page boundary
	_page_frames = (uint64 *)mem_alloc(_page_count / 8);
	// Clear bitset
	mem_fill((uint8 *)_page_frames, _page_count / 8, 0);

	// Determine unusable memory regions
	uint64 paddr_from;
	uint64 paddr_to;
	for (i = 0; i < mem_map->size; i ++){
		if (mem_map->entries[i].type != kMemOk){
			// Mark all unusable regions used
			paddr_from = (mem_map->entries[i].base & PAGE_MASK);
			paddr_to = ((mem_map->entries[i].base + mem_map->entries[i].length) & PAGE_MASK);
			while (paddr_from < paddr_to){
				page_set_frame(paddr_from);
				paddr_from += PAGE_SIZE;
			}
		}
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
	vaddr = PAGE_CANONICAL(vaddr);
	uint8 i = 4;
	uint64 idx = PAGE_PML_IDX(vaddr, i);
	for (i = 3; i >= 1; i --){
		if (!table[idx].s.present){
			// Next level table does not exist - create one
			ct = (pm_t *)mem_alloc_align(sizeof(pm_t) * 512);
			mem_fill((uint8 *)ct, sizeof(pm_t) * 512, 0);

			// Store the physical address
			table[idx].raw = (uint64)ct;
			// Set the flags
			table[idx].s.present = 1;
			table[idx].s.writable = 1;
			table[idx].s.write_through = 1;
			if (mmio){
				table[idx].s.cache_disable = 1;
			}
			// Continiue with the newly created table
			table = ct;
		} else {
			// Find next level table address
			table = (pm_t *)PAGE_ADDRESS(table, idx);
		}
		// Find next level index
		idx = PAGE_PML_IDX(vaddr, i);
	}
	// Last level table
	if (!table[idx].s.present){
		// Store physical address
		table[idx].raw = (paddr & PAGE_MASK);
		// Set the flags
		table[idx].s.present = 1;
		table[idx].s.writable = 1;
		table[idx].s.write_through = 1;
		if (mmio){
			table[idx].s.cache_disable = 1;
		}
		return true;
	}
	return false;
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
		if (table[idx].s.present){
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

uint64 page_alloc(){
	// TODO: create smarter one
	uint64 vaddr = page_placeholder;
#if DEBUG == 1
	debug_print(DC_WB, "Try to allocate: %x", vaddr);
#endif
	if (!page_resolve(vaddr)){
		if (page_map(vaddr, vaddr)){
			page_placeholder += PAGE_SIZE;
			return vaddr;
		}
	}

	return 0;	
}
void page_free(uint64 vaddr){
	pm_t *table = (pm_t *)page_get_pml4();
	uint8 i = 4;
	uint64 idx = PAGE_PML_IDX(vaddr, i);
	for (i = 3; i >= 1; i --){
		if (table[idx].s.present){
			// Next level table exists - load it's address
			table = (pm_t *)PAGE_ADDRESS(table, idx);
			// Get next level index from this table
			idx = PAGE_PML_IDX(vaddr, i);
		} else {
			return;
		}
	}
	table[idx].s.present = 0;
	uint64 paddr = table[idx].raw & PAGE_MASK;
	page_clear_frame(paddr);
	// TODO: check if some page table is empty and delete it's parent
}

uint64 page_get_pml4(){
	return get_cr3();
}

void page_set_pml4(uint64 paddr){
	set_cr3(paddr);
}