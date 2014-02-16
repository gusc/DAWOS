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

#include "pic.h"
#include "io.h"

void pic_init(){
    // Send ICW1
	// bit 0 - we'll send ICW4
	// bit 4 - we're initializing PIC
	outb(PICM_CMD, 0x11); // Initialize master PIC
	outb(PICS_CMD, 0x11); // Initialize slave PIC
	// Send ICW2
	outb(PICM_DATA, 0x20); // Send IRQ 0-7 to interrupts 32-39
	outb(PICS_DATA, 0x28); // Send IRQ 8-15 to interrupts 40-47
	// Send ICW3
	outb(PICM_DATA, 0x04); // Tell Master PIC that Slave PIC is at IRQ2
	outb(PICS_DATA, 0x02); // Tell Slave PIC that it's cascaded to IRQ2
	// Send ICW4
	outb(PICM_DATA, 0x01); // Enable 80x86 mode
	outb(PICS_DATA, 0x01); // Enable 80x86 mode
    // Enable interrupts
    pic_enable(0xFFFE);
}
void pic_enable(uint16 irq_mask){
	irq_mask = (~irq_mask);
	uint8 irq_m = (uint8)irq_mask;
	uint8 irq_s = (uint8)(irq_mask >> 8);
	outb(PICS_DATA, irq_s);
	outb(PICM_DATA, irq_m);
}
void pic_disable(){
	outb(PICS_DATA, 0xFF);
	outb(PICM_DATA, 0xFF);
}
uint16 pic_read_ocw3(uint8 ocw3){
	outb(PICM_CMD, ocw3);
    outb(PICS_CMD, ocw3);
    return (inb(PICS_CMD) << 8) | inb(PICM_CMD);
}
void pic_eoi(uint64 irq){
	if (irq >= 8){
		outb(PICS_CMD, PIC_EOI);
	}
	outb(PICM_CMD, PIC_EOI);
}
