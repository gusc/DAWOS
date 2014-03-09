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

#ifndef __ata_h
#define __ata_h

#include "common.h"
#include "interrupts.h"

#define IDE_ATA        0x00
#define IDE_ATAPI      0x01
 
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

// IDE channel IO addresses

#define IDE_PRIMARY         0x1F0
#define IDE_PRIMARY_CTRL    0x3F6
#define IDE_SECONDARY       0x170
#define IDE_SECONDARY_CTRL  0x376

// ATA registers

#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// Status registers

#define ATA_SR_BSY      0x80
#define ATA_SR_DRDY     0x40
#define ATA_SR_DF       0x20
#define ATA_SR_DSC      0x10
#define ATA_SR_DRQ      0x08
#define ATA_SR_CORR     0x04
#define ATA_SR_IDX      0x02
#define ATA_SR_ERR      0x01

// Error registers

#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

// ATA commands

#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// ATAPI commands

#define ATAPI_CMD_READ            0xA8
#define ATAPI_CMD_EJECT           0x1B

#define ATA_IDENT_DEVICETYPE   0
#define ATA_IDENT_CYLINDERS    2
#define ATA_IDENT_HEADS        6
#define ATA_IDENT_SECTORS      12
#define ATA_IDENT_SERIAL       20
#define ATA_IDENT_MODEL        54
#define ATA_IDENT_CAPABILITIES 98
#define ATA_IDENT_FIELDVALID   106
#define ATA_IDENT_MAX_LBA      120
#define ATA_IDENT_COMMANDSETS  164
#define ATA_IDENT_MAX_LBA_EXT  200

typedef struct {
    uint16 base;
    uint16 control;
    uint16 bmide;
    uint8 no_int;
} ide_chan_t;

typedef struct {
    struct {
        uint64 active           :1; // 0 - device is no connected, 1 - device is up-n-running
        uint64 slave            :1; // 1 = device is slave
        uint64 atapi            :1; // 1 = device is ATAPI, 0 = ATA
        uint64 reserved         :61;
    } status;
    uint64 sectors;
    uint32 commands;
    uint16 capabilities;
    uint16 signature;
    uint8 channel;
    uint8 model[41];
} ata_dev_t;

// ATA IDENTIFY response data (ATA8 specs)

typedef struct {
    struct {
       uint16 reserved          :1;
       uint16 retired1          :1;
       uint16 incomplete        :1; // Response is incomplete
       uint16 retired2          :3;
       uint16 obsolete          :2;
       uint16 retired3          :7;
       uint16 ata_dev           :1; // 0 = ATA device
    } conf;                         // General configuration (mostly obsolete)
    uint16 obsolete1;
    uint16 specific;                // Specific configuration (optional)
    uint16 obsolete2;
    uint16 retired1[4];
    uint16 obsolete3;
    uint16 cf_assoc[2];             // Reserved for CompactFlash association
    uint16 retired2;
    uint8 serial[20];               // Serial number
    uint16 retired3[2];
    uint16 obsolete4;
    uint8 fw_version[8];            // Firmware version
    uint8 model[30];                // Model number
    struct {
        uint16 sect_num         :8; // Maximum number of sectors transfered in one DRQ (0 = reserved)
        uint16 reserved         :8; // Always 0x80
    } drq_cap;                      // DRQ capabilites
    struct {
        uint16 tc_sup           :1; // Trusted computing feature set supported
        uint16 tc_res           :13;// Reserved for TC group
        uint16 one              :1; // Always one
        uint16 zero             :1; // Always zero
    } tc_opt;                       // Trusted computing feature options
    struct {
        uint32 retired          :8;
        uint32 dma_sup          :1; // DMA supported
        uint32 lba_sup          :1; // LBA supported
        uint32 iordy_dis        :1; // IORDY may be disabled
        uint32 iordy_sup        :1; // IORDY supported (1 = yes, 0 = maybe?)
        uint32 reserved1        :1;
        uint32 stby_sup         :1; // Standby timer supported as by the specs (1 = yes, 0 = device managed)
        uint32 reserved2        :2;
        uint32 stby_vendor      :1; // If 1 - vendor specific standby time minimum is used
        uint32 obsolete         :1;
        uint32 reserved3        :12;
        uint32 one              :1; // Always one
        uint32 zero             :1; // Always zero
    } cap;                          // Capabilities
    uint16 obsolete5[2];
    struct {
        uint8 obsolete          :1;
        uint8 f64_valid         :1; // Field (64:70) is valid
        uint8 f80_valid         :1; // Field 80 is valid
        uint8 reserved          :5;
    } field_valid;                  // Field validity
    uint8 freefall_sens;            // Freefall control sensitivity
    uint16 obsolete6[5];
    uint16 drq_sectors;             // Number of sectors transfered per DRQ
    uint32 lba_lo;                  // Maximum LBA (low dword)
    uint16 obsolete7;
    uint16 dma_modes;               // DMA modes (supported and selected)
    uint16 pio_modes;               // PIO modes (supported)
    uint16 dma_cycles;              // DMA cycle time in nanoseconds
    uint16 dma_cycles_cec;          // Manufacturers recomended DMA cycle time (ns)
    uint16 pio_cycles_wof;          // PIO transfer cycle time (without flow control)
    uint16 pio_cycles_wf;           // PIO transfer cycle time (with IORDY flow control)
    uint16 reserved1[2];
    uint16 reserved2[4];
    uint16 queue_depth;             // Queue depth (bits 0:4)
    uint16 sata_cap;                // Serial ATA capabilities
    uint16 reserved3;
    uint16 sata_fs;                 // Serial ATA features supported
    uint16 sata_fe;                 // Serial ATA features enabled
    uint32 version;                 // Version number (Minor.Major - don't forget to swap)
    struct {
        uint16 smart            :1; // S.M.A.R.T. supported
        uint16 security         :1; // Security feature supported
        uint16 obsolete1        :1;
        uint16 pm               :1; // Mandatory Power Management feature supported
        uint16 packet           :1; // PACKET feature supported
        uint16 write_cache      :1; // Volatile write cache supported
        uint16 look_ahead       :1; // Read look-ahead supported
        uint16 release_int      :1; // Release interrupt supported
        uint16 service_int      :1; // SERVICE interrupt supported
        uint16 dev_reset        :1; // DEVICE RESET supported
        uint16 hpa              :1; // HPA feature set supported
        uint16 obsolete2        :1;
        uint16 write_buffer     :1; // WRITE BUFFER supported
        uint16 read_buffer      :1; // READ BUFFER supported
        uint16 nop              :1; // NOT supported
        uint16 obsolete3        :1;
    } ata_feat1;                    // Commands and features supported
    struct {
        uint16 dl_mcode         :1; // DOWNLOAD MICROCODE supported
        uint16 tcq              :1; // TCQ feature set supported
        uint16 cfa              :1; // CFA feature set supported
        uint16 apm              :1; // APM feature set supported
        uint16 obsolete         :1;
        uint16 puis             :1; // PUIS feature set supported
        uint16 set_feat         :1; // SET FEATURES is equired to spin-up after after power-up
        uint16 reserved         :1;
        uint16 set_max          :1; // SET MAX scurity extension is supported
        uint16 aam              :1; // AAM feature set supported
        uint16 lba48            :1; // 48bit LBA is supported
        uint16 dco              :1; // DCO feature set supported
        uint16 flush_cahce      :1; // Mandatory FLUSH CAHCE is supported
        uint16 flush_cahce_ext  :1; // FLUSH CAHCE EXT is supported
        uint16 one              :1; // Always one
        uint16 zero             :1; // Always zero
    } ata_feat2;                    // Commands and features supported
    struct {
        uint16 smart_log        :1; // SMART logging supported
        uint16 smart_test       :1; // SMART self-test supported
        uint16 media_sn         :1; // Media serial number supported
        uint16 mcpt             :1; // Media Card Pass Throuhg command set supported
        uint16 streaming        :1; // Streaming feature set supported
        uint16 gpl              :1; // GPL feature set supported
        uint16 fua_ext          :1; // WRITE DMA FUA EXT and WRITE MULTIPLE FUA EXT supported
        uint16 qfua_ext         :1; // WRITE DMA QUEUED FUA EXT supported
        uint16 obsolete         :2;
        uint16 reserved         :2;
        uint16 idle             :1; // The IDLE IMMEDIATE command with UNLOAD feature is supported
        uint16 one              :1; // Always one
        uint16 zero             :1; // Always zero
    } ata_feat3;                    // Commands and features supported
    struct {
        uint16 smart            :1; // S.M.A.R.T. enabled
        uint16 security         :1; // Security feature enabled
        uint16 obsolete1        :1;
        uint16 pm               :1; // Mandatory Power Management feature supported
        uint16 packet           :1; // PACKET feature supported
        uint16 write_cache      :1; // Volatile write cache enabled
        uint16 look_ahead       :1; // Read look-ahead enabled
        uint16 release_int      :1; // Release interrupt enabled
        uint16 service_int      :1; // SERVICE interrupt enabled
        uint16 dev_reset        :1; // DEVICE RESET supported
        uint16 hpa              :1; // HPA feature set supported
        uint16 obsolete2        :1;
        uint16 write_buffer     :1; // WRITE BUFFER supported
        uint16 read_buffer      :1; // READ BUFFER supported
        uint16 nop              :1; // NOT supported
        uint16 obsolete3        :1;
    } ata_feat4;                    // Commands and features supported or enabled
    struct {
        uint16 dl_mcode         :1; // DOWNLOAD MICROCODE supported
        uint16 tcq              :1; // TCQ feature set supported
        uint16 cfa              :1; // CFA feature set supported
        uint16 apm              :1; // APM feature set enabled
        uint16 obsolete         :1;
        uint16 puis             :1; // PUIS feature set enabled
        uint16 set_feat         :1; // SET FEATURES is equired to spin-up after after power-up
        uint16 reserved1        :1;
        uint16 set_max          :1; // SET MAX scurity extension is supported
        uint16 aam              :1; // AAM feature set enabled
        uint16 lba48            :1; // 48bit LBA is supported
        uint16 dco              :1; // DCO feature set supported
        uint16 flush_cahce      :1; // Mandatory FLUSH CAHCE is supported
        uint16 flush_cahce_ext  :1; // FLUSH CAHCE EXT is supported
        uint16 reserved2        :1;
        uint16 w119_valid       :1; // Words (119:120) are valid
    } ata_feat5;                    // Commands and features supported or enabled
    struct {
        uint16 smart_log        :1; // SMART logging supported
        uint16 smart_test       :1; // SMART self-test supported
        uint16 media_sn         :1; // Media serial number supported
        uint16 mcpt             :1; // Media Card Pass Throuhg command set supported
        uint16 streaming        :1; // Streaming feature set supported
        uint16 gpl              :1; // GPL feature set supported
        uint16 fua_ext          :1; // WRITE DMA FUA EXT and WRITE MULTIPLE FUA EXT supported
        uint16 qfua_ext         :1; // WRITE DMA QUEUED FUA EXT supported
        uint16 obsolete         :2;
        uint16 reserved         :2;
        uint16 idle             :1; // The IDLE IMMEDIATE command with UNLOAD feature is supported
        uint16 one              :1; // Always one
        uint16 zero             :1; // Always zero
    } ata_feat6;                    // Commands and features supported and enabled
    uint16 udma_modes;              // UltraDMA modes
    uint16 erase_time;              // Time required for Normal Erase mode SECURITY ERASE UNIT command
    uint16 eerase_time;             // Time required for an Enhanced Erase mode SECURITY ERASE UNIT command
    uint16 apm_level;               // Current APM level value
    uint16 mpi;                     // Master Password Identifier
    struct {
        uint16 one1             :1; // Always one
        uint16 dev0_id_method   :2; // These bits indicate how Device0 determined the device number: (01 = a jumper was used. 10 = the CSEL signal was used.)
        uint16 dev0_diag        :1; // Device0 diagnostics status
        uint16 dev0_pdiag       :1; // Device0 assertion status (PDIAG)
        uint16 dev0_dasp        :1; // Device0 assertion status (DASP)
        uint16 dev0_resp_dev1   :1; // Device0 responds when Device1 selected
        uint16 reserved1        :1;
        uint16 one2             :1; // Always one
        uint16 dev1_id_method   :2; // These bits indicate how Device1 determined the device number: (01 = a jumper was used. 10 = the CSEL signal was used.)
        uint16 dev1_pdiag       :1; // Device1 assertion status (PDIAG)
        uint16 reserved2        :1;
        uint16 dev_cblid        :1; // 1 = device detected CBLID- above V iHB, 0 = device detected CBLID- below V iLB
        uint16 one3             :1; // Always one
        uint16 zero             :1; // Always zero
    } hw_reset;                     // Hardware reset result.
    uint16 aam;                     // Current AAM value
    uint16 stream_min_req;          // Stream minimum request size
    uint16 stream_time_dma;         // Stream transfer time (DMA)
    uint16 stream_latency;          // Stream access latency (DMA and PIO)
    uint32 stream_gran;             // Streaming Performance Granularity
    uint32 lba_hi;                  // Total Number of User Addressable Logical Sectors for 48-bit commands
    uint16 stream_time_pio;         // Stream transfer time (PIO)
    uint16 reserved4;
    struct {
        uint16 power            :4; // 2^X logical sectors per physical sector
        uint16 reserved         :8;
        uint16 ls_256           :1; // Device Logical Sector longer than 256 Words
        uint16 multi_ls         :1; // Device has multiple logical sectors per physical sector
        uint16 one              :1; // Always one
        uint16 zero             :1; // Always zero
    } sector_size;
    uint16 issd;                    // Inter-seek delay for ISO 7779 standard acoustic testing
    uint8 wwname[8];                // World wide name
    uint16 reserved5[4];
    uint16 reserved6;
    uint32 ls_size;                 // Logical sector size
    struct {
        uint16 x;
    } ata_feat7;                    // Commands and feature sets supported (Continued from words 84:82)
    struct {
        uint16 x;
    } ata_feat8;                    // Commands and feature sets supported (Continued from words 84:82)
} ata_identify_t;

/**
* Initialize ATA drives
*/
bool ata_init();
/**
* Get the number of ATA devices connected
* @return number of ATA devices
*/
uint8 ata_num_device();
/**
* Get the device information
* @param device - destination device structure
* @param idx - device index
* @return true on success
*/
bool ata_device_info(ata_dev_t *device, uint8 idx);
/**
* Read data from ATA drive
* @param buff - destination buffer
* @param idx - device index
* @param lba - starting LBA of read
* @param len - number of bytes to read
* @return true on success
*/
bool ata_read(uint8 *buff, uint8 idx, uint64 lba, uint64 len);
/**
* ATA IRQ handler
*/
uint64 ata_handler(irq_stack_t *stack);

#if DEBUG == 1
/**
* List available ATA devices on screen
*/
void ata_list();
#endif

#endif
