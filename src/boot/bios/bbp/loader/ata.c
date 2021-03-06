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

#include "lib.h"
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
      ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | (_ide_chan[channel].no_int << 1));
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
      ata_write_reg(channel, ATA_REG_CONTROL, (_ide_chan[channel].no_int << 1));
   }
}

static uint8 ata_read_reg(uint8 channel, uint8 reg) {
   unsigned char result;
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | (_ide_chan[channel].no_int << 1));
   }
   sleep(1);
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
      ata_write_reg(channel, ATA_REG_CONTROL, (_ide_chan[channel].no_int << 1));
   }
   return result;
}

static void ata_read_buffer(uint8 channel, uint8 reg, uint32 *buffer, uint64 quads) {
   if (reg > 0x07 && reg < 0x0C){
      ata_write_reg(channel, ATA_REG_CONTROL, 0x80 | (_ide_chan[channel].no_int << 1));
   }
   sleep(1);
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
      ata_write_reg(channel, ATA_REG_CONTROL, (_ide_chan[channel].no_int << 1));
   }
}
/**
* Initialize IDE controller
*/
static void ata_init_ide(uint32 bar0, uint32 bar1, uint32 bar2, uint32 bar3, uint32 bar4){
    // Store IO adresses - primary channel
    _ide_chan[_ide_chan_count].base = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
    _ide_chan[_ide_chan_count].control = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
    _ide_chan[_ide_chan_count].bmide = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
    _ide_chan[_ide_chan_count].no_int = 0;
    // Disable IRQs
    ata_write_reg(_ide_chan_count, ATA_REG_CONTROL, 2);

    _ide_chan_count ++;

    // Store IO adresses - secondary channel
    _ide_chan[_ide_chan_count].base = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
    _ide_chan[_ide_chan_count].control = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
    _ide_chan[_ide_chan_count].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE
    _ide_chan[_ide_chan_count].no_int = 0;
    // Disable IRQs
    ata_write_reg(_ide_chan_count, ATA_REG_CONTROL, 2);

    _ide_chan_count ++;
}
/**
* Initialize all ATA devices on IDE port
* @param i - IDE port
*/
static void ata_init_dev(uint8 i){
    uint8 j, k;
    for (j = 0; j < 2; j++) {
        uint8 err = 0;
        uint8 type = IDE_ATA;
        uint8 status;
        _ata_dev[_ata_dev_count].status.active = 0; // Assuming that no drive here.
 
        // Select the drive on channel
        ata_write_reg(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4)); // Select Drive.
        sleep(1);

        // Send ATA IDENTIFY command
        ata_write_reg(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
        sleep(1);

        // Polling
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
 
        // Probe for ATAPI device
        uint8 atapi = 0;
        if (err != 0) {
            uint8 cl = ata_read_reg(i, ATA_REG_LBA1);
            uint8 ch = ata_read_reg(i, ATA_REG_LBA2);
 
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
 
        // Read Identification Space of the device
        uint8 *ata_ident = (uint8 *)mem_alloc(sizeof(uint16) * 256);
        ata_read_buffer(i, ATA_REG_DATA, (uint32 *)ata_ident, 128);
 
        // Read device parameters
        _ata_dev[_ata_dev_count].status.active = 1;
        _ata_dev[_ata_dev_count].status.atapi = atapi;
        _ata_dev[_ata_dev_count].channel = i;
        _ata_dev[_ata_dev_count].status.slave = j;
        _ata_dev[_ata_dev_count].signature = *((uint16 *)(ata_ident + ATA_IDENT_DEVICETYPE));
        _ata_dev[_ata_dev_count].capabilities = *((uint32 *)(ata_ident + ATA_IDENT_CAPABILITIES));
        _ata_dev[_ata_dev_count].commands1  = *((uint64 *)(ata_ident + ATA_IDENT_COMMANDSETS));
        _ata_dev[_ata_dev_count].commands2  = (uint64)(*((uint32 *)(ata_ident + ATA_IDENT_COMMANDSETS + 8)));
        _ata_dev[_ata_dev_count].sector_size = 512;
        uint16 sectors = *(uint16 *)(ata_ident + ATA_IDENT_SECTOR_SIZE);
        if ((sectors & 0x4000) && !(sectors & 0x8000)){
            _ata_dev[_ata_dev_count].status.multisect = ((sectors & 0x2000) ? 1 : 0);
            _ata_dev[_ata_dev_count].status.largesect = ((sectors & 0x1000) ? 1 : 0);
            if (_ata_dev[_ata_dev_count].status.largesect){
                sectors &= 0xF;
                _ata_dev[_ata_dev_count].sector_size = 512 * (uint64)(2 << sectors);
            }
        }

        // Get device size:
        if (_ata_dev[_ata_dev_count].commands1 & (1 << 26)){
            // Device uses 48-Bit Addressing:
            _ata_dev[_ata_dev_count].sectors = ((uint64)(*((uint64 *)(ata_ident + ATA_IDENT_MAX_LBA_EXT))) & 0xFFFFFFFFFFFF);
            _ata_dev[_ata_dev_count].status.lba48 = 1;
        } else {
            // Device uses CHS or 28-bit Addressing:
            _ata_dev[_ata_dev_count].sectors = ((uint64)(*((uint32 *)(ata_ident + ATA_IDENT_MAX_LBA))) & 0xFFFFFFF);
            _ata_dev[_ata_dev_count].status.lba48 = 0;
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
/**
* Poll and check for errors
* @param channel - IDE channel index
* @param err_check - do status checking
* @retun error number or 0 on success
*/
static uint64 ata_poll(uint8 channel, uint8 err_check){
    uint64 spins = 0;
    // Wait at least 400ns before reading status
    sleep(1);
    // Polling
    while (ata_read_reg(channel, ATA_REG_STATUS) & ATA_SR_BSY){
        spins ++;
    }
    // Read status and do status checks
    if (err_check){
        uint8 status = ata_read_reg(channel, ATA_REG_STATUS);
        if (status & ATA_SR_ERR){
            return 1; // Error
        }
        if (status & ATA_SR_DF){
            return 2; // Device Fault
        }
        spins = 0;
        // BSY = 0; DF = 0; ERR = 0 so we should check for DRQ now.
        while(!(ata_read_reg(channel, ATA_REG_STATUS) & ATA_SR_DRQ)){
            spins ++;
            if (spins > 10000){
                return 3;
                break;
            }
        }
    }
    return 0;
}
/**
* Select ATA device
* @param idx - ATA device index
*/
static void ata_dev_sel(uint8 idx){
    ata_write_reg(_ata_dev[idx].channel, ATA_REG_HDDEVSEL, 0xE0 | (_ata_dev[idx].status.slave << 4));
    sleep(1);
}

bool ata_init(){
    uint8 dev_count = pci_num_device(0x01, 0x01);
    uint8 i;
    pci_addr_t addr;

    _ide_chan = (ide_chan_t *)mem_alloc_clean(sizeof(ide_chan_t) * 2 * dev_count);
    _ide_chan_count = 0;
    _ata_dev_count = 0;
    for (i = 0; i < dev_count; i ++){
        if (pci_get_device(&addr, 0x01, 0x01, 0)){
            pci_device_t *dev = (pci_device_t *)mem_alloc_clean(sizeof(pci_device_t));
            pci_get_config(dev, addr);
            ata_init_ide(dev->bar[0], dev->bar[1], dev->bar[2], dev->bar[3], dev->bar[4]);
        }
    }

    if (_ide_chan_count > 0){
        interrupt_reg_irq_handler(14, &ata_handler);
        interrupt_reg_irq_handler(15, &ata_handler);
        _ata_dev = (ata_dev_t *)mem_alloc_clean(sizeof(ata_dev_t) * 2 * _ide_chan_count);
        _ata_dev_count = 0;
        for (i = 0; i < _ide_chan_count; i ++){
            ata_init_dev(i);
            // Re-enable interrupts
            _ide_chan[i].no_int = 0;
            ata_write_reg(i, ATA_REG_CONTROL, 0);
        }
        if (_ata_dev_count > 0){
            return true;
        }
    }
    return false;
}

uint8 ata_num_device(){
    return _ata_dev_count;
}

bool ata_device_info(ata_dev_t *device, uint8 idx){
    if (idx < _ata_dev_count){
        mem_copy((uint8 *)device, (uint8 *)&_ata_dev[idx], sizeof(ata_dev_t));
        return true;
    }
    return false;
}
bool ata_read(uint8 *buff, uint8 idx, uint64 lba, uint64 len){
    if (idx < _ata_dev_count){
        uint64 err = 0;
        bool error = false;
        uint64 i = 0;
        uint8 channel = _ata_dev[idx].channel;
        uint64 sectors = (len + (_ata_dev[idx].sector_size - 1)) / _ata_dev[idx].sector_size;

        // Select drive
        ata_dev_sel(idx);

        // Prepare request
        if (_ata_dev[idx].status.lba48){
            // For 48-bit LBA we push high 24bits first
            ata_write_reg(channel, ATA_REG_SECCOUNT1, (uint8)(sectors >> 8));
            ata_write_reg(channel, ATA_REG_LBA3, (uint8)(lba >> 24));
            ata_write_reg(channel, ATA_REG_LBA4, (uint8)(lba >> 32));
            ata_write_reg(channel, ATA_REG_LBA5, (uint8)(lba >> 40));
        }
        ata_write_reg(channel, ATA_REG_SECCOUNT0, (uint8)sectors);
        ata_write_reg(channel, ATA_REG_LBA0, (uint8)lba);
        ata_write_reg(channel, ATA_REG_LBA1, (uint8)(lba >> 8));
        ata_write_reg(channel, ATA_REG_LBA2, (uint8)(lba >> 16));

        // Send PIO read command
        if (_ata_dev[idx].status.lba48){
            ata_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_READ_PIO_EXT);
        } else {
            ata_write_reg(channel, ATA_REG_COMMAND, ATA_CMD_READ_PIO);
        }
        
        // Prepare temporary read buffer
        uint8 *tmp = (uint8 *)mem_alloc(sectors * _ata_dev[idx].sector_size);
        uint8 *t = tmp;
        for (i = 0; i < sectors; i ++){
            // Wait for device to become ready
            err = ata_poll(channel, 1);
            if (!err){
                // Read data from device
                insw(_ide_chan[channel].base, (uint16 *)t, (_ata_dev[idx].sector_size >> 1));
                t += _ata_dev[idx].sector_size;
            } else {
                error = true;
                break;
            }
        }
        // Copy temporary data to destination buffer
        mem_copy(buff, tmp, len);
        mem_free(tmp);
        if (!error){
            return true;
        }
    }
    return false;
}

bool ata_write(uint8 idx, uint8 *buff, uint64 lba, uint64 len){
    // TODO: implement
    return false;
}

uint64 ata_handler(irq_stack_t *stack){
    //debug_print(DC_WB, "ATA IRQ %d", (uint64)stack->irq_no);
    return 0;
}

#if DEBUG == 1
void ata_list(){
    uint8 i;
    debug_print(DC_WB, "IDE channels: %d", (uint64)_ide_chan_count);
    for (i = 0; i < _ide_chan_count; i ++){
        debug_print(DC_WB, "   Channel %d: %x", (uint64)i, (uint64)_ide_chan[i].base);
    }
    debug_print(DC_WB, "ATA devices: %d", (uint64)_ata_dev_count);
    for (i = 0; i < _ata_dev_count; i ++){
        uint8 channel = _ata_dev[i].channel;
        debug_print(DC_WB, "ATA drive %d @0x%x (%s%s%s%s)", (uint64)i, (uint64)_ide_chan[channel].base, 
            (_ata_dev[i].status.slave ? "Slave" : "Master"),
            (_ata_dev[i].status.atapi ? ", ATAPI" : ""),
            (_ata_dev[i].status.lba48 ? ", LBA48" : ""),
            (_ata_dev[i].status.largesect ? ", Large Sector" : ""));
        //debug_print(DC_WB, "    Commands 1 %x", (uint64)_ata_dev[i].commands1);
        //debug_print(DC_WB, "    Commands 2 %x", (uint64)_ata_dev[i].commands2);
        //debug_print(DC_WB, "    Capabilities %x", (uint64)_ata_dev[i].capabilities);
        debug_print(DC_WB, "    Size %d (%d x %d)", (uint64)_ata_dev[i].sectors * (uint64)_ata_dev[i].sector_size, 
            (uint64)_ata_dev[i].sectors, (uint64)_ata_dev[i].sector_size);
        debug_print(DC_WB, "    Model %s", (uint64)_ata_dev[i].model);
    }
}
#endif