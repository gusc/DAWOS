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

#include "../config.h"
#include "pit.h"
#include "io.h"
#if DEBUG == 1
    #include "debug_print.h"
#endif

uint16 _counter = 0;
uint8 _mode = 0;
uint64 _ticks = 0;

void pit_init(){
    // Initialize PIT to work with 1ms intervals
    pit_set(1193, PIT_MODE_RATE);
    
    interrupt_reg_irq_handler(0, &pit_handler);
}
uint16 pit_current_count(){
    outb(PIT_CMD, PIT_CMD_LATCH);
    return inw(PIT_CH0);
}
uint16 pit_get_counter(){
    return _counter;
}
uint8 pit_get_mode(){
    return _mode;
}
uint64 pit_get_ticks(){
    return _ticks;
}
void pit_reset(){
    uint8 cmd = PIT_CMD_RELOAD;
    cmd |= (_mode << 1);

    outb(PIT_CMD, cmd);
    outb(PIT_CH0, (uint8)_counter);
    outb(PIT_CH0, (uint8)(_counter >> 8));

    _ticks = 0;
}
void pit_set(uint16 counter, uint8 mode){
    mode &= 0x7;
    _mode = mode;
    _counter = counter;
    if (_counter == 0){
        _counter = 0xFFFF;
    }

    pit_reset();
}
uint64 pit_handler(irq_stack_t *stack){
    _ticks ++;
    return 0;
}
