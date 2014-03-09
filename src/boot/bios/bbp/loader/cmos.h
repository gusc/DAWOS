/*

Helper function for CMOS operations
===================================

ASM wrappers

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
#ifndef __cmos_h
#define __cmos_h

#include "common.h"
#include "io.h"

#define CMOS_RTC_SECONDS 0x00
#define CMOS_RTC_MINUTES 0x02
#define CMOS_RTC_HOURS 0x04
#define CMOS_RTC_WEEKDAY 0x06
#define CMOS_RTC_DAY 0x07
#define CMOS_RTC_MONTH 0x08
#define CMOS_RTC_YEAR 0x09
#define CMOS_RTC_CENTURY 0x32
#define CMOS_RTC_STATUS_A 0x0A
#define CMOS_RTC_STATUS_B 0x0B

#define CMOS_RTC_STATUS_24H 0x02
#define CMOS_RTC_STATUS_BIN 0x04

/**
* Read CMOS register
* @param reg - register number
* @return register value
*/
static inline uint8 cmos_read(uint8 reg){
	outb(0x70, reg);
	return inb(0x71);
}
/**
* Write CMOS register
* @param reg - register number
* @param value - value to write
*/
static inline void cmos_write(uint8 reg, uint8 value){
	outb(0x70, reg);
	outb(0x71, value);
}

#endif