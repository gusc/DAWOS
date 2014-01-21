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

// Page masks
#define PAGE_IMASK         (PAGE_SIZE - 1) // Inverse mask
#define PAGE_MASK          (~PAGE_IMASK)
/**
* Align address to page start boundary
* @param n - address to align
* @return aligned address
*/
#define PAGE_ALIGN(n) (n & PAGE_MASK)
/**
* Align address to page end boundary
* @param n - address to align
* @return aligned address
*/
#define PAGE_SIZE_ALIGN(n) ((n + PAGE_IMASK) & PAGE_MASK)

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
* Map a physical address to virtual address
* @param paddr - physical address to map
* @param vaddr - virtual address to map to
* @return true on success, false if virtual address is already taken
*/
bool page_map(uint64 paddr, uint64 vaddr);
/**
* Release the free address
* @param vaddr - virtual address to release
* @return true on success, false if virtual address was not mapped
*/
bool page_release(uint64 vaddr);
/**
* Identity map a physical address for memory maped IO (no cache!)
* @param paddr - physical address to map
* @param vaddr - virtual address to map to
* @return true on success, false if virtual address is already taken
*/
bool page_map_mmio(uint64 paddr, uint64 vaddr);
/**
* Resolve physical address from virtual addres
* @param vaddr - virtual address to resolve
* @return physical address
*/
uint64 page_resolve(uint64 vaddr);
/**
* Allocate a new page
* @return virtual address
*/
uint64 page_alloc(uint64 vaddr, uint64 size);
/**
* Free the page mapped to virtual address
* @param vaddr - virtual address to free
*/
void page_free(uint64 vaddr, uint64 size);
/**
* Get the current physical address of PML4
* @return physical address of PML4
*/
uint64 page_get_pml4();
/**
* Set new physical address of PML4
* @param paddr - new physical address of PML4
*/
void page_set_pml4(uint64 paddr);

#endif /* __paging_h */