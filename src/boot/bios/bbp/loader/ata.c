/*
ATA functions
=============


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

#include "ata.h"
#include "pci.h"
#include "memory.h"
#if DEBUG == 1
    #include "debug_print.h"
#endif

bool ata_init(){
    uint8 dev_count = pci_num_device(0x01, 0x01);
    
    debug_print(DC_WB, "IDE count: %d", dev_count);

    if (dev_count > 0){
        pci_addr_t addr = pci_get_device(0x01, 0x01, 0);
    
        debug_print(DC_WB, "    Addr: 0x%x", addr.raw);

        if (addr.raw != 0){
            pci_device_t *dev = (pci_device_t *)mem_alloc(sizeof(pci_device_t));
            pci_get_config(dev, addr);

            debug_print(DC_WB, "    BAR0: 0x%x", dev->bar[0]);
            debug_print(DC_WB, "    BAR1: 0x%x", dev->bar[1]);
            debug_print(DC_WB, "    BAR2: 0x%x", dev->bar[2]);
            debug_print(DC_WB, "    BAR3: 0x%x", dev->bar[3]);
            debug_print(DC_WB, "    BAR4: 0x%x", dev->bar[4]);
        }
    }

    return false;
}

