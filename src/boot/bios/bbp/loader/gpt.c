/*

GUID partition table functions
==============================

Read and write partition GPT disks

License (BSD-3)
===============

Copyright (c) 2013, Gusts 'gusC' Kaksis <gusts.kaksis@gmail.com>
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

#include "gpt.h"
#include "lib.h"
#include "memory.h"
#include "ata.h"
#if DEBUG == 1
#include "debug_print.h"
#endif

static gpt_header_t *_gpt_disks[4];
static gpt_part_entry_t *_gpt_part[4];
static uint64 _part_count[4];

static bool gpt_test_sign(char *sign){
    char test[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};
    for (uint8 i = 0; i < 8; i ++){
        if (sign[i] != test[i]){
            return false;
        }
    }
    return true;
}
static bool guid_compare(guid_t *guid1, guid_t *guid2){
    if (guid1->data1 != guid2->data1){
        return false;
    }
    if (guid1->data2 != guid2->data2){
        return false;
    }
    if (guid1->data3 != guid2->data3){
        return false;
    }
    for (uint8 i = 0; i < 8; i ++){
        if (guid1->data4[i] != guid2->data4[i]){
            return false;
        }
    }
    return true;
}

void gpt_init(){
    for (uint64 i = 0; i < ata_num_device(); i ++){
        _gpt_disks[i] = (gpt_header_t *)mem_alloc_clean(sizeof(gpt_header_t));
        _gpt_part[i] = (gpt_part_entry_t *)mem_alloc_clean(sizeof(gpt_part_entry_t) * 128);
        _part_count[i] = 0;
    }
}

bool gpt_init_drive(uint64 drive){
    bool found = false;
    guid_t empty_guid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

    if (drive < ata_num_device()){
        mbr_t *mbr = (mbr_t *)mem_alloc_clean(sizeof(mbr_t));
        if (ata_read((uint8 *)mbr, drive, 0, 512)){
            for (uint64 i = 0; i < 4; i ++){
                if (mbr->part[i].type == 0xEE){
                    // Looks like a GPT - GPT protective partition entry found
                    if (ata_read((uint8 *)_gpt_disks[drive], drive, 1, sizeof(gpt_header_t))){    
                        if (gpt_test_sign(_gpt_disks[drive]->signature)){
                            // GPT signature is OK
                            if (ata_read((uint8 *)_gpt_part[drive], drive, _gpt_disks[drive]->part_arr_lba, sizeof(gpt_part_entry_t) * _gpt_disks[drive]->part_item_count)){
                                found = true;
                                for (uint64 j = 0; j < _gpt_disks[drive]->part_item_count; j ++){
                                    if (!guid_compare(&_gpt_part[drive][j].part_guid, &empty_guid)){
                                        _part_count[drive] ++;
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
        mem_free(mbr);
    }
    return found;
}

uint64 gpt_num_part(uint64 drive){
    return _part_count[drive];
}

bool gpt_part_entry(gpt_part_entry_t *entry, uint64 drive, uint64 part){
    guid_t empty_guid = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

    if (drive < ata_num_device()){
        if (part < _part_count[drive]){
            for (uint64 j = 0; j < _gpt_disks[drive]->part_item_count; j ++){
                uint64 i = 0;
                if (!guid_compare(&_gpt_part[drive][j].part_guid, &empty_guid)){
                    if (i == part){
                        mem_copy((uint8 *)entry, (uint8 *)&_gpt_part[drive][j], sizeof(gpt_part_entry_t));
                        return true;
                    }
                    i ++;
                }
            }
        }
    }
    return false;
}