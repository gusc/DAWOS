/*
PIT functions
=============

Programmable Interval Timer

Channel 0 is the only one we're using here, so there are no channel selectors implemented

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

#ifndef __pit_h
#define __pit_h

#include "common.h"
#include "interrupts.h"

#define PIT_CH0     0x40    // Channel 0 data port (read/write)
#define PIT_CH1     0x41    // Channel 1 data port (read/write)
#define PIT_CH2     0x42    // Channel 2 data port (read/write)
#define PIT_CMD     0x43    // Mode/Command register (write only, a read is ignored)

#define PIT_CMD_STATUS  0xE2    // Read-back command to get the status (Channel 0 only)
#define PIT_CMD_LATCH   0x30    // Command to latch current counter
#define PIT_CMD_RELOAD  0x30    // Command to reload PIT (set mode and counter)

#define PIT_MODE_ONE    0x00    // Mode 0 - Interrupt On Terminal Count (a.k.a one-shot)
#define PIT_MODE_RATE   0x02    // Mode 2 - Rate generator
#define PIT_MODE_SQUARE 0x03    // Mode 3 - Square generator
#define PIT_MODE_STROBE 0x04    // Mode 4 - Square generator

/**
* Initialize PIT
*/
void pit_init(uint16 pit_counter);
/**
* Read the current PIT counter value
* @return status
*/
uint16 pit_current_count();
/**
* Read the counter value from PIT
* @return counter
*/
uint16 pit_get_counter();
/**
* Read the operating mode of PIT
* @return counter
*/
uint8 pit_get_mode();
/**
* Write a new counter value and operating mode to PIT
* @param counter - counter value
* @param mode - operating mode (0 - 5)
*/
void pit_set(uint16 counter, uint8 mode);
/**
* Reset PIT with previous values
*/
void pit_reset();
/**
* Get the number of ticks
* @return number of ticks since the PIT was last re-set
*/
uint64 pit_get_ticks();
/**
* PIT interrupt handler
* @param stack - pointer to IRQ stack
* @return 1 if interrupt could not be handled
*/
uint64 pit_handler(irq_stack_t *stack);

#endif
