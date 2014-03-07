/*
Sleep routine
=============

Simple PIT based sleep routine

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
#include "sleep.h"
#include "pit.h"
#if DEBUG == 1
	#include "debug_print.h"
#endif

void isleep(uint64 iter){
	uint32 x = 1;
	while(x < iter){
		x ++;
	}
}

void sleep(uint64 time){
    uint64 counter = (uint64)pit_get_counter();
    if (counter > 0){
        uint64 ticks_per_ms = PIT_FREQ / (counter * 1000);
        if (ticks_per_ms == 0){
            // We need at least one tick per ms
            ticks_per_ms = 1;
        }
        uint64 tick_count = time * ticks_per_ms;
        uint64 tick_start = pit_get_ticks();
        uint64 tick = tick_start;
        while (tick < tick_start + tick_count){
            tick = pit_get_ticks();
            NOP();
            NOP();
            NOP();
            NOP();
            NOP();
            NOP();
            NOP();
            NOP();
        }
    }
    //debug_print(DC_WB, "Sleep - s: %d, e: %d, t: %d", tick_start, tick, tick_count);
}
