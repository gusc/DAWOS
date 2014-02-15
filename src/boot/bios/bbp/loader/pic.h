/*
PIC functions
=============

Programmable Interrupt Controller

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

#ifndef __pic_h
#define __pic_h

#include "common.h"

// PIC IO addresses

#define PICM_CMD                    0x20
#define PICM_DATA                   0x21
#define PICS_CMD                    0xA0
#define PICS_DATA                   0xA1

// PIC commands

#define PIC_EOI						0x20	// End-of-interrupt command code
#define PIC_READ_IRR                0x0a    // OCW3 irq ready next CMD read
#define PIC_READ_ISR                0x0b    // OCW3 irq service next CMD read

/**
* Enable PIC
* @param irq_mask - IRQ mask (0-15 bits for IRQs 0-15)
*/
void pic_enable(uint16 irq_mask);
/**
* Disable PIC
*/
void pic_disable();
/**
* Read data from PIC
* @param ocw3 - OCW3 bits
* @return value
*/
uint16 pic_read_ocw3(uint8 ocw3);
/**
* Send End-of-interrupt to PIC
* @param irq - IRQ number
*/
void pic_eoi(uint64 irq);

#endif
