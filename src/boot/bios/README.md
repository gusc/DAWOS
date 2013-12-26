Legacy BIOS bootloader
======================

This source branch contains legacy BIOS bootloader, that gets loaded by BIOS from MBR (Master Boot Record).
DAW OS relies only on GPT structures, so this is kind of a hybrid MBR code that loads BIOS Boot Partition code as a second
stage bootloader

Structure
---------

* bbp - BIOS Boot partition
* mbr - Master Boot Record
* rules.mk - build rules for BIOS bootloader

Memory map (Physical)
---------------------

0x0000000000000000 - 0x00000000000FFFFF = Legacy memory area
           0x00000 -            0x005FF = Reserved for BIOS
           0x00600 -            0x006FF = Bootloader data
           0x00700 -            0x007FF = MBR bootstrap after relocation
           0x00800 -            0x009FF = E820 Memory map
           0x00900 -            0x07BFF = Bootloader stack
           0x07C00 -            0x6CFFF = Bootloader code
		   0x6D000 -            0x7FFFF = PML tables to map first 32MB of RAM to do a Long Mode jump
           ...D000 -            ...DFFF = PML4 table
           ...E000 -            ...EFFF = PML3 table
           ...F000 -            ...FFFF = PML2 table
           ..70000 -            ..7FFFF = 16x PML1 tables
           0x80000 -            0x9FFFF = Reserved for BIOS
           0xA0000 -            0xB7FFF = VGA video buffer
           0xB8000 -            0xBFFFF = VGA teletype buffer
           0xC0000 -            0xFFFFF = Reserved for BIOS

0x0000000000100000 -           0x200000 = Placement heap area (used in the initial phase of page expansion)
0x0000000000200000 - ...                = Free to use (used by heap allocator)