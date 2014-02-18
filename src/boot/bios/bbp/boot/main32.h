/*

Protected Mode initialization
=============================

This file contains initialization code protected mode (preparation to switch to 
long mode).

It does:
    * page setup and initialization

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

#ifndef __main32_h
#define __main32_h

#include "common32.h"

/**
* 64-bit page table/directory/level3/level4 entry structure
*/

typedef struct {
    uint64 present          : 1;    // Is the page present in memory?
    uint64 writable         : 1;    // Is the page writable?
    uint64 user             : 1;    // Is the page for userspace?
    uint64 write_through    : 1;    // Do we want write-trough? (when cached, this also writes to memory)
    uint64 cache_disable    : 1;    // Disable cache on this page?
    uint64 accessed         : 1;    // Has the page been accessed by software?
    uint64 dirty            : 1;    // Has the page been written to since last refresh? (ignored in PML4E, PML3E, PML2E)
    uint64 pat              : 1;    // Page attribute table (in PML1E), 
                                    // page size bit (must be 0 in PML4E, in PML3E 1 = 1GB page size, in PML2E 1 = 2MB page size otherwise 4KB pages are used)
    uint64 global           : 1;    // Is the page global? (ignored in PML4E, PML3E, PML2E)
    uint64 data             : 3;    // Ignored (ignored in all PML levels)
    uint64 frame            : 40;    // Frame address (4KB aligned)
    uint64 data2            : 11;    // Ignored (ignored in all PML levels)
    uint64 xd               : 1;    // Execute disable bit (whole region is not accessible by instruction fetch)
} pm_t;

// Page masks
#define PAGE_IMASK          (PAGE_SIZE - 1) // Inverse mask
#define PAGE_MASK           (~PAGE_IMASK)
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

// Make virtual address canonical (sign extend)
#define PAGE_CANONICAL(va) ((va << 16) >> 16)
// Page table entry index mask
#define PAGE_PML_IDX_MASK 0x1FF
// Page offset mask
#if PAGE_LEVELS == 2
    #define PAGE_OFFSET_MASK 0x3FFFFF
#elif PAGE_LEVELS == 3
    #define PAGE_OFFSET_MASK 0x1FFFFF
#else
    #define PAGE_OFFSET_MASK 0xFFF
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

#endif /* __main32_h */
