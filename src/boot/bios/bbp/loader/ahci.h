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

#ifndef __ahci_h
#define __ahci_h

#include "common.h"
#include "../config.h"

/**
* Initialize AHCI driver
* @return false if no AHCI controller has been found
*/
bool ahci_init();
/**
* Get the number of AHCI devices connected
* @return number of devices available
*/
uint64 ahci_num_dev();
/**
* Read data from AHCI drive
* @param idx - device index in the device list
* @param addr - start address on the drive
* @param buff - byte buffer to write into
* @param len - number of bytes to read
* @todo implement
* @return false if read failed
*/
bool ahci_read(uint64 idx, uint64 addr, uint8 *buff, uint64 len);
/**
* Write data to AHCI drive
* @param idx - device index in the device list
* @param addr - start address on the drive
* @param buff - byte buffer to read from
* @param len - number of bytes to write
* @todo implement
* @return false if write failed
*/
bool ahci_write(uint64 idx, uint64 addr, uint8 *buff, uint64 len);

#if DEBUG == 1
void ahci_list();
#endif

#endif
