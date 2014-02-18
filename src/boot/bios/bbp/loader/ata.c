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
#include "io.h"
#include "sleep.h"
#if DEBUG == 1
    #include "debug_print.h"
#endif

static ide_chan_t *_ide_chan;
static uint8 _ide_chan_count;

static ata_dev_t *_ata_dev;
static uint8 _ata_dev_count;

static void ata_write_reg(uint8 channel, uint8 reg, uint8 data) {
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | _ide_chan[channel].no_int);
   }
   if (reg < 0x08){
      outb(_ide_chan[channel].base  + reg - 0x00, data);
   } else if (reg < 0x0C){
      outb(_ide_chan[channel].base  + reg - 0x06, data);
   } else if (reg < 0x0E){
      outb(_ide_chan[channel].control  + reg - 0x0A, data);
   } else if (reg < 0x16){
      outb(_ide_chan[channel].bmide + reg - 0x0E, data);
   }
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, _ide_chan[channel].no_int);
   }
}

static uint8 ata_read_reg(uint8 channel, uint8 reg) {
   unsigned char result;
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | _ide_chan[channel].no_int);
   }
   if (reg < 0x08){
      result = inb(_ide_chan[channel].base + reg - 0x00);
   } else if (reg < 0x0C){
      result = inb(_ide_chan[channel].base  + reg - 0x06);
   } else if (reg < 0x0E){
      result = inb(_ide_chan[channel].control  + reg - 0x0A);
   } else if (reg < 0x16){
      result = inb(_ide_chan[channel].bmide + reg - 0x0E);
   }
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, _ide_chan[channel].no_int);
   }
   return result;
}

void ata_read_buffer(uint8 channel, uint8 reg, uint32 *buffer, uint64 quads) {
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | _ide_chan[channel].no_int);
   }
   //asm("pushw %es; movw %ds, %ax; movw %ax, %es");
   if (reg < 0x08){
      insd(_ide_chan[channel].base  + reg - 0x00, buffer, quads);
   } else if (reg < 0x0C){
      insd(_ide_chan[channel].base  + reg - 0x06, buffer, quads);
   } else if (reg < 0x0E){
      insd(_ide_chan[channel].control  + reg - 0x0A, buffer, quads);
   } else if (reg < 0x16){
      insd(_ide_chan[channel].bmide + reg - 0x0E, buffer, quads);
   }
   //asm("popw %es;");
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, _ide_chan[channel].no_int);
   }
}

void ata_init_ide(uint32 bar0, uint32 bar1, uint32 bar2, uint32 bar3, uint32 bar4){
    // Store IO adresses - primary channel
    _ide_chan[_ide_chan_count].base = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
    _ide_chan[_ide_chan_count].control = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
    _ide_chan[_ide_chan_count].bmide = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    _ide_chan[_ide_chan_count].no_int = 0;
    // Disable IRQs
    ata_write_reg(_ide_chan_count, ATA_REG_CONTROL, 2);
            
    debug_print(DC_WB, "    PRIMARY: 0x%x", _ide_chan[_ide_chan_count].base);

    _ide_chan_count ++;

    // Store IO adresses - secondary channel
    _ide_chan[_ide_chan_count].base = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
    _ide_chan[_ide_chan_count].control = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
    _ide_chan[_ide_chan_count].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE
    _ide_chan[_ide_chan_count].no_int = 0;
    // Disable IRQs
    ata_write_reg(_ide_chan_count, ATA_REG_CONTROL, 2);
            
    debug_print(DC_WB, "    SECONDARY: 0x%x", _ide_chan[_ide_chan_count].base);

    _ide_chan_count ++;
}

void ata_init_dev(uint8 i){
    uint8 j, k;
    for (j = 0; j < 2; j++) {
        unsigned char err = 0, type = IDE_ATA, status;
        _ata_dev[_ata_dev_count].status.active = 0; // Assuming that no drive here.
 
        // (I) Select Drive:
        ata_write_reg(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
        sleep(1); // Wait 1ms for drive select to work.
 
        // (II) Send ATA Identify Command:
        ata_write_reg(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
        sleep(1); // This function should be implemented in your OS. which waits for 1 ms.
                // it is based on System Timer Device Driver.
 
        // (III) Polling:
        if (ata_read_reg(i, ATA_REG_STATUS) == 0) {
            continue; // If Status = 0, No Device.
        }
 
        while(1) {
            status = ata_read_reg(i, ATA_REG_STATUS);
            if ((status & ATA_SR_ERR)) {
                err = 1;
                break;  // If Err, Device is not ATA.
            }
            if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ)) {
                break; // Everything is right.
            }
        }
 
        // (IV) Probe for ATAPI Devices:
        uint8 atapi = 0;
        if (err != 0) {
            unsigned char cl = ata_read_reg(i, ATA_REG_LBA1);
            unsigned char ch = ata_read_reg(i, ATA_REG_LBA2);
 
            if (cl == 0x14 && ch ==0xEB){
                atapi = IDE_ATAPI;
            } else if (cl == 0x69 && ch == 0x96){
                atapi = IDE_ATAPI;
            } else { 
                continue; // Unknown Type (may not be a device).
            }
 
            ata_write_reg(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
            sleep(1);
        }
 
        // (V) Read Identification Space of the Device:
        uint16 *ata_ident = (uint16 *)mem_alloc(sizeof(uint16) * 256);
        ata_read_buffer(i, ATA_REG_DATA, (uint32 *)ata_ident, 128);
 
        // (VI) Read Device Parameters:
        _ata_dev[_ata_dev_count].status.active = 1;
        _ata_dev[_ata_dev_count].status.atapi = atapi;
        _ata_dev[_ata_dev_count].channel = &_ide_chan[i];
        _ata_dev[_ata_dev_count].status.master = j;
        _ata_dev[_ata_dev_count].signature = *((uint16 *)(ata_ident + ATA_IDENT_DEVICETYPE));
        _ata_dev[_ata_dev_count].capabilities = *((uint16 *)(ata_ident + ATA_IDENT_CAPABILITIES));
        _ata_dev[_ata_dev_count].commands  = *((uint32 *)(ata_ident + ATA_IDENT_COMMANDSETS));
 
        // (VII) Get Size:
        if (_ata_dev[_ata_dev_count].commands & (1 << 26)){
            // Device uses 48-Bit Addressing:
            _ata_dev[_ata_dev_count].sectors   = (uint64)(*((uint32 *)(ata_ident + ATA_IDENT_MAX_LBA_EXT)));
        } else {
            // Device uses CHS or 28-bit Addressing:
            _ata_dev[_ata_dev_count].sectors   = (uint64)(*((uint32 *)(ata_ident + ATA_IDENT_MAX_LBA)));
        }
 
        // (VIII) String indicates model of device (like Western Digital HDD and SONY DVD-RW...):
        for(k = 0; k < 40; k += 2) {
            _ata_dev[_ata_dev_count].model[k] = ata_ident[ATA_IDENT_MODEL + k + 1];
            _ata_dev[_ata_dev_count].model[k + 1] = ata_ident[ATA_IDENT_MODEL + k];
        }
        _ata_dev[_ata_dev_count].model[40] = 0; // Terminate String.
 
        _ata_dev_count++;
    }
}

bool ata_init(){
    uint8 dev_count = pci_num_device(0x01, 0x01);
    uint8 i;
    debug_print(DC_WB, "IDE count: %d", (uint64)dev_count);

    _ide_chan = (ide_chan_t *)mem_alloc(sizeof(ide_chan_t) * 2 * dev_count);
    _ide_chan_count = 0;
    _ata_dev_count = 0;
    for (i = 0; i < dev_count; i ++){
        pci_addr_t addr = pci_get_device(0x01, 0x01, 0);

        debug_print(DC_WB, "    Addr: 0x%x", (uint64)addr.raw);

        if (addr.raw != 0){
            pci_device_t *dev = (pci_device_t *)mem_alloc(sizeof(pci_device_t));
            pci_get_config(dev, addr);

            ata_init_ide(dev->bar[0], dev->bar[1], dev->bar[2], dev->bar[3], dev->bar[4]);
        }
    }

    debug_print(DC_WB, "IDE channels: %d", (uint64)_ide_chan_count);

    if (_ide_chan_count > 0){

        interrupt_reg_irq_handler(14, &ata_handler);
        interrupt_reg_irq_handler(15, &ata_handler);

        _ata_dev = (ata_dev_t *)mem_alloc(sizeof(ata_dev_t) * 2 * _ide_chan_count);
        _ata_dev_count = 0;
        for (i = 0; i < _ide_chan_count; i ++){
            ata_init_dev(i);
        }

        debug_print(DC_WB, "ATA devices: %d", (uint64)_ata_dev_count);
        for (i = 0; i < _ata_dev_count; i ++){
            debug_print(DC_WB, "ATA drive @0x%x (%s)", (uint64)_ata_dev[i].channel->base, (_ata_dev[i].status.master ? "Master" : "Slave"));
            debug_print(DC_WB, "    Size %d", (uint64)_ata_dev[i].sectors);
            debug_print(DC_WB, "    Model %s", (uint64)_ata_dev[i].model);
        }

        if (_ata_dev_count > 0){
            return true;
        }
    }
    return false;
}


uint64 ata_handler(irq_stack_t *stack){
    debug_print(DC_WB, "ATA IRQ %d", (uint64)stack->irq_no);
    return 0;
}