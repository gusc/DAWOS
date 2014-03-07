/*

AHCI driver
===========


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
#include "lib.h"
#include "pci.h"
#include "paging.h"
#include "memory.h"
#include "ahci.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

static ahci_dev_t *_ahci_dev;
static uint64 _ahci_dev_count = 0;

// Check device type
static uint32 ahci_get_type(ahci_port_t *port){
	if (port->ssts.det != 3){
		return AHCI_DEV_NONE;
	}
	if (port->ssts.ipm != 1){
		return AHCI_DEV_NONE;
	}
	switch (port->sig){
		case AHCI_DEV_SATAPI:
		case AHCI_DEV_SEMB:
		case AHCI_DEV_PM:
			return port->sig;
		default:
			return AHCI_DEV_SATA;
	}
}
static void ahci_init_port(ahci_hba_t *hba, pci_device_t *dev){
	uint32 dev_type = 0;
	uint32 ports = hba->pi;
	uint8 i = 0;
	for (i = 0; i < 32; i ++){
		if (ports & 1) {
			dev_type = ahci_get_type(&hba->ports[i]);
            switch (dev_type){
				case AHCI_DEV_SATA:
				case AHCI_DEV_SATAPI:
				case AHCI_DEV_SEMB:
				case AHCI_DEV_PM:
                    // Initialize AHCI mode and set interrupt-enable bit
					hba->ghc.ae = 1;
					hba->ghc.ie = 1;
                    // Store into our SATA cache
					_ahci_dev[_ahci_dev_count].hba = hba;
					_ahci_dev[_ahci_dev_count].port = i;
                    _ahci_dev[_ahci_dev_count].int_pin = dev->int_pin;
                    _ahci_dev[_ahci_dev_count].int_line = dev->int_line;
                    _ahci_dev[_ahci_dev_count].cmd = dev->header.command;
                    _ahci_dev[_ahci_dev_count].sts = dev->header.status;
					_ahci_dev_count ++;
					break;
			}
		}
		ports >>= 1;
	}
}

bool ahci_init(){
	uint64 abar = 0;
	uint8 i = 0;
	uint64 offset = 0;
	uint8 dev_count = 0;
	ahci_hba_t *hba;
    pci_addr_t addr;

	pci_device_t *dev = (pci_device_t *)mem_alloc_clean(sizeof(pci_device_t));
	_ahci_dev = (ahci_dev_t *)mem_alloc_clean(sizeof(ahci_dev_t) * 256);
    _ahci_dev_count = 0;

	dev_count = pci_num_device(0x1, 0x6);
    debug_print(DC_WB, "Dev count %d", dev_count);
	if (dev_count > 0){
		for (i = 0; i < dev_count; i ++){
			if (pci_get_device(&addr, 0x1, 0x6, i)){
                debug_print(DC_WB, "PCI addr: %x", addr.raw);
			    // Get AHCI controller configuration
				pci_get_config(dev, addr);
				// Get ABAR (AHCI Base Address)
				abar = ((uint64)(dev->bar[5])) & AHCI_HBA_MASK;
                // Map the page
				for (offset = 0; offset < AHCI_HBA_SIZE; offset += PAGE_SIZE){
					page_map_mmio(abar + offset, abar + offset);
				}
				hba = (ahci_hba_t *)abar;

                debug_print(DC_WB, "SATA controller at %u:%u", addr.s.bus, addr.s.device);
	            debug_print(DC_WB, "     ABAR:0x%x", abar);
	            debug_print(DC_WB, "     Num Ports:%d", hba->cap.np + 1);
	            debug_print(DC_WB, "     Num Commands:%d", hba->cap.ncs + 1);
	            debug_print(DC_WB, "     Version:%x", hba->vs);
	

				//if (hba->cap.s64a == 1){
					ahci_init_port(hba, dev);
				//}
			}
		}
	}
	if (_ahci_dev_count > 0){
		return true;
	}
	return false;
}

static int8 ahci_free_slot(ahci_hba_t *hba, ahci_port_t *port){
	// If not set in SACT and CI, the slot is free
	int8 i = 0;
	uint32 slots = (port->sact | port->ci);
	for (; i < hba->cap.ncs; i++) { 
		if ((slots & 1) == 0) {
			return i;
		}
		slots >>= 1;
	}
	return -1;
}

uint64 ahci_num_dev(){
	return _ahci_dev_count;
}

bool ahci_read(uint64 idx, uint64 addr, uint8 *buff, uint64 len){
	if (idx < _ahci_dev_count){
		ahci_hba_t *hba = _ahci_dev[idx].hba;
		ahci_port_t *port = &hba->ports[_ahci_dev[idx].port];
		uint64 spin = 0;
		int8 slot = ahci_free_slot(hba, port);

		if (slot == -1){
			return false;
		}

		ahci_hba_cmd_header_t *cmd = (ahci_hba_cmd_header_t *)port->clb;
        page_map_mmio((uint64)cmd, (uint64)cmd);
		cmd += slot;
		cmd->desc.cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32);			// Command FIS size
		cmd->desc.w = 0;												// Read from device
		cmd->desc.c = 1;
		cmd->desc.prdtl = (uint16)((len - 1) / AHCI_BLOCK_SIZE) + 1;	// PRDT entries count

		ahci_hba_cmd_tbl_t *tbl = (ahci_hba_cmd_tbl_t *)cmd->ctba;
        page_map_mmio((uint64)tbl, (uint64)tbl);
		mem_fill((uint8 *)tbl, sizeof(ahci_hba_cmd_tbl_t) + ((cmd->desc.prdtl - 1) * sizeof(ahci_hba_prdt_entry_t)), 0);

		uint64 i = 0;
		uint64 count = len / AHCI_BLOCK_SIZE;
		// Read first entries (if any)
		for (; i < cmd->desc.prdtl - 1; i ++){
			tbl->prdt_entry[i].dba = (uint64)buff;
			tbl->prdt_entry[i].dbc = AHCI_BLOCK_SIZE;
			tbl->prdt_entry[i].i = 1;
			buff += AHCI_BLOCK_SIZE;
			len -= AHCI_BLOCK_SIZE;
		}
		// Last entry
		tbl->prdt_entry[i].dba = (uint64)buff;
		tbl->prdt_entry[i].dbc = len;		// Leftover size
		tbl->prdt_entry[i].i = 1;
 
		// Setup command
		fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(tbl->cfis);
        page_map_mmio((uint64)fis, (uint64)fis);
		fis->fis_type = FIS_TYPE_REG_H2D;
		fis->cmd = 1;						// Command
		fis->command = ATA_CMD_READ_DMA_EX;
 
		fis->lba0 = (uint8)addr;
		fis->lba1 = (uint8)(addr>>8);
		fis->lba2 = (uint8)(addr>>16);
		fis->device = 1 << 6;				// LBA mode
 
		fis->lba3 = (uint8)(addr>>24);
		fis->lba4 = (uint8)(addr>>32);
		fis->lba5 = (uint8)(addr>>40);
 
		fis->count = count;
 
		// The below loop waits until the port is no longer busy before issuing a new command
		while ((port->tfd.status.busy | port->tfd.status.drq) && spin < 1000000) {
			spin++;
		}
		if (spin == 1000000){
			return false;
		}
	
        port->ci = (1 << slot);				        // Issue command
		if (!port->cmd.st){
			if (port->cmd.fre){
                port->cmd.st = 1;					// Start processing
			} else {
				return false;
			}
		}

        // Wait for completion
		spin = 0;
		while (1) {
			// In some longer duration reads, it may be helpful to spin on the DPS bit 
			// in the PxIS port field as well (1 << 5)
			if (spin >= 1000000){
				break;
			}
			spin ++;
			if ((port->ci & (1 << slot)) == 0) {
				break;
			}
			if (port->is.tfes) {
				// Task file error
				return false;
			}
		}

		if (spin == 1000000){
			debug_print(DC_WB, "Spinnout");
		}
 
		// Check again
		if (port->is.tfes){
			return false;
		}
 
		return true;
	}
	return false;
}
bool ahci_write(uint64 idx, uint64 addr, uint8 *buff, uint64 len){
	if (idx < _ahci_dev_count){
		ahci_hba_t *hba = _ahci_dev[idx].hba;
		ahci_port_t *port = &hba->ports[_ahci_dev[idx].port];

	}
	return false;
}
bool ahci_id(uint64 idx, uint8 *buff){
    if (idx < _ahci_dev_count){
		ahci_hba_t *hba = _ahci_dev[idx].hba;
		ahci_port_t *port = &hba->ports[_ahci_dev[idx].port];
		uint64 spin = 0;
		int8 slot = ahci_free_slot(hba, port);

		if (slot == -1){
			return false;
		}

        ahci_hba_cmd_header_t *cmd = (ahci_hba_cmd_header_t *)port->clb;
		cmd += slot;
        cmd->desc.cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32);			// Command FIS size
		cmd->desc.w = 0;												// Read from device
		cmd->desc.c = 1;
		cmd->desc.prdtl = 1;	// PRDT entries count

		ahci_hba_cmd_tbl_t *tbl = (ahci_hba_cmd_tbl_t *)cmd->ctba;
		mem_fill((uint8 *)tbl, sizeof(ahci_hba_cmd_tbl_t), 0);

        // Last entry
		tbl->prdt_entry[0].dba = (uint64)buff;
		tbl->prdt_entry[0].dbc = 512;		// Leftover size
		tbl->prdt_entry[0].i = 1;

        fis_reg_h2d_t *fis = (fis_reg_h2d_t *)(tbl->cfis);
		fis->fis_type = FIS_TYPE_REG_H2D;
		fis->command = ATA_CMD_IDENTIFY;
        fis->device = 0;
        fis->cmd = 1;
        port->ci = (1 << slot);				        // Issue command
		if (!port->cmd.st){
			if (port->cmd.fre){
                port->cmd.st = 1;					// Start processing
			} else {
				return false;
			}
		}

        // Wait for completion
		spin = 0;
		while (1) {
			// In some longer duration reads, it may be helpful to spin on the DPS bit 
			// in the PxIS port field as well (1 << 5)
			if (spin >= 1000000){
				break;
			}
			spin ++;
			if ((port->ci & (1 << slot)) == 0) {
				break;
			}
			if (port->is.tfes) {
				// Task file error
				return false;
			}
		}

		if (spin == 1000000){
			debug_print(DC_WB, "Spinnout");
		}
 
		// Check again
		if (port->is.tfes){
			return false;
		}
 
		return true;
	}
	return false;
}

#if DEBUG == 1
void ahci_list(){
	uint64 i = 0;
	/*
	debug_print(DC_WB, "SATA controller at %u:%u", addr.s.bus, addr.s.device);
	debug_print(DC_WB, "     ABAR:0x%x", abar);
	debug_print(DC_WB, "     Num Ports:%d", hba->cap.np + 1);
	debug_print(DC_WB, "     Num Commands:%d", hba->cap.ncs + 1);
	debug_print(DC_WB, "     Version:%x", hba->vs);
	*/
	ahci_dev_t *dev;
	uint32 dev_type = 0;
	for (; i < _ahci_dev_count; i ++){
		dev = &_ahci_dev[i];
		dev_type = ahci_get_type(&(dev->hba->ports[dev->port]));
		switch (dev_type){
			case AHCI_DEV_SATA:
				debug_print(DC_WGR, "SATA drive found at port %d", dev->port);
				break;
			case AHCI_DEV_SATAPI:
				debug_print(DC_WGR, "SATAPI drive found at port %d", dev->port);
				break;
			case AHCI_DEV_SEMB:
				debug_print(DC_WGR, "SEMB drive found at port %d", dev->port);
				break;
			case AHCI_DEV_PM:
				debug_print(DC_WGR, "PM drive found at port %d", dev->port);
				break;
			default:
				debug_print(DC_WGR, "Unknown at port %d, %x", dev->port, (uint64)dev_type);
				break;
		}
		debug_print(DC_WGR, "     FIS base @0x%x", dev->hba->ports[dev->port].fb);
		debug_print(DC_WGR, "     CL base  @0x%x", dev->hba->ports[dev->port].clb);
        debug_print(DC_WGR, "     INT pin: %d, line: %d", dev->int_pin, dev->int_line);
        debug_print(DC_WGR, "     CMD: %x, STS: %x", dev->cmd, dev->sts);
	}
}
#endif
