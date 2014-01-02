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

#ifndef __paging_h
#define __paging_h

#include "common.h"
#include "interrupts.h"

typedef union {
	struct {
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
	} s;
	uint64 raw;							// Raw value
} pm_t;

typedef union {
	struct {
		uint64 offset			: 12;	// Offset from the begining of page
		uint64 page_idx			: 9;	// Page index (in pml1)
		uint64 table_idx		: 9;	// Table index (in pml2)
		uint64 directory_idx	: 9;	// Directory index (in pml3)
		uint64 drawer_idx		: 9;	// Drawer index (in pml4)
		uint64 canonical		: 16;	// Should be FFF... if drawer_idx 9th bit is 1 (see: canonical address)
	} s;
	uint64 raw;
} vaddr_t;

// Page masks
#define PAGE_IMASK         (PAGE_SIZE - 1) // Inverse mask
#define PAGE_MASK          (~PAGE_IMASK)
// Align address to page start boundary
#define PAGE_ALIGN(n) (n & PAGE_MASK)
// Align size to page end boundary
#define PAGE_SIZE_ALIGN(n) ((n + PAGE_IMASK) & PAGE_MASK)
// Make virtual address canonical (sign extend)
#define PAGE_CANONICAL(va) ((va << 16) >> 16)

/**
* Initialize paging
*/
void page_init();
/**
* Page fault handler
* @param stack - pointer to interrupt stack
* @return 1 if interrupt could not be handled
*/
uint64 page_fault(int_stack_t *stack);
/**
* Get total installed RAM
* @return RAM size in bytes
*/
uint64 page_total_mem();
/**
* Get total available RAM
* @return RAM size in bytes
*/
uint64 page_available_mem();
/**
* Normalize virtual address to canonical form
* Usefull when converting from 32bit addresses to 64bit
* @param vaddr - virtual address to normalize
* @return normalized virtual address
*/
uint64 page_normalize_vaddr(uint64 vaddr);
/**
* Identity map a physical address
* @param paddr - physical address to map
* @return virtual address
*/
uint64 page_map(uint64 paddr);
/**
* Identity map a physical address for memory maped IO (no cache!)
* @param paddr - physical address to map
* @return virtual address
*/
uint64 page_map_mmio(uint64 paddr);
/**
* Resolve physical address from virtual addres
* @param vaddr - virtual address to resolve
* @return physical address
*/
uint64 page_resolve(uint64 vaddr);
/**
* Allocate a new page
* @param size - size in bytes to allocate (will be aligned to page size)
* @return virtual address
*/
uint64 page_alloc(uint64 size);
/**
* Free the page mapped to virtual address
* @param vaddr - virtual address to free
*/
void page_free(uint64 vaddr);


#endif /* __paging_h */