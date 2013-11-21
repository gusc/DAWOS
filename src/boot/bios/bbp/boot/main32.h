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

typedef union {
	struct {
		uint64 present			: 1;	// Is the page present in memory?
		uint64 writable			: 1;	// Is the page writable?
		uint64 user				: 1;	// Is the page for userspace?
		uint64 write_through	: 1;	// Do we want write-trough? (when cached, this also writes to memory)
		uint64 cache_disable	: 1;	// Disable cache on this page?
		uint64 accessed			: 1;	// Has the page been accessed by software?
		uint64 dirty			: 1;	// Has the page been written to since last refresh?
		uint64 pat				: 1;	// Is the page a PAT? (dunno what it is)
		uint64 global			: 1;	// Is the page global? (dunno what it is)
		uint64 data				: 3;	// Available for kernel use (do what you want?)
		uint64 frame			: 52;	// Frame address (shifted right 12 bits)
	} s;
	uint64 raw;							// Raw value
} pm_t;

#define PAGE_MASK		0xFFFFFFFFF000;

#endif /* __main32_h */
