/*

Loader entry point
==================

This is where the fun part begins

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
#include "main64.h"
#include "lib.h"
#include "io.h"
#include "interrupts.h"
#include "paging.h"
#include "memory.h"
#include "pci.h"
#include "pit.h"
#include "pic.h"
#include "ata.h"
#include "ahci.h"
#include "sleep.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

extern uint32 _checksum;
extern uint64 _end;

/**
* Loader entry point
*/
void main64(){

#if DEBUG == 1
	// Clear the screen
	debug_clear(DC_WB);
	// Show something on the screen
	debug_print(DC_WB, "Booting...");
#endif
    uint64 check = (uint64)(&_checksum);
    if (check == 0xF00BAA){
        //debug_print(DC_WB, "Bootloader: 0x7E00 - 0x%x", (uint64)&_end);

        // Disable interrupts
        interrupt_disable();
        // Initialize memory manager
        mem_init();
        // Initialize PIC
        pic_init();
        // Initialize interrupts
        interrupt_init();
        // Initialize paging (well, actually re-initialize)
        page_init();
        // Initialize kernel heap allocator
        mem_init_heap(HEAP_MAX_SIZE);
        // Initialize PIT
        pit_init(PIT_COUNTER);
        // Enable all IRQs
        pic_enable(0xFFFF);
        // Enable interrupts (Do it after PCI, otherwise it seems to GPF at random)
        interrupt_enable();
        // Initialize PCI
        if (pci_init()){
	        // Initialize ATA
            if (ata_init()){
#if DEBUG == 1
		        ata_list();
#endif
                uint8 *mem = (uint8 *)mem_alloc_clean(512);
                uint64 dev_count = ata_num_device();
                uint64 y = 0;
                if (dev_count > 0){
                    debug_print(DC_WB, "Read:");
                    if (ata_read(mem, 0, 0, 512)){
                        for (y = 0; y < 64; y += 8){
					        debug_print(DC_WB, "%x %x %x %x %x %x %x %x", mem[y], mem[y + 1], mem[y + 2], mem[y + 3], mem[y + 4], mem[y + 5], mem[y + 6], mem[y + 7]);
				        }
                    } else {
                        debug_print(DC_WB, "Failed");
                    }
                }

            }
	        // Initialize AHCI
            //debug_print(DC_WB, "AHCI init");
        
            /*if (ahci_init()){
#if DEBUG == 1
		        ahci_list();
#endif
		        uint8 *mem = (uint8 *)mem_alloc_clean(512);
		        uint64 dev_count = ahci_num_dev();
		        uint64 y = 23;
                uint16 isr = pic_read_ocw3(PIC_READ_ISR);
		        if (dev_count > 0){
                    debug_print(DC_WB, "ISR: %x", (uint64)isr);
                    if (ahci_id(0, mem)){
                        debug_print(DC_WB, "ID OK");
                        debug_print(DC_WB, "%x %x %x %x %x %x %x %x", mem[y], mem[y + 1], mem[y + 2], mem[y + 3], mem[y + 4], mem[y + 5], mem[y + 6], mem[y + 7]);
                    } else {
                        debug_print(DC_WB, "ID failed");
                    }
                    mem_fill(mem, 512, 0);
			        if (ahci_read(0, 0, mem, 512)){
				        debug_print(DC_WB, "Read:");
				        for (y = 0; y < 64; y += 8){
					        debug_print(DC_WB, "%x %x %x %x %x %x %x %x", mem[y], mem[y + 1], mem[y + 2], mem[y + 3], mem[y + 4], mem[y + 5], mem[y + 6], mem[y + 7]);
				        }
			        } else {
				        debug_print(DC_WB, "Read failed");
			        }
                }
	        }*/
        }
#if DEBUG == 1
	    debug_print(DC_WB, "Done");
#endif
    } else {
#if DEBUG == 1
	    debug_print(DC_WB, "Wrong checksum");
#endif
    }
	// Infinite loop
	while(true){}
}
