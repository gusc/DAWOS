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
           0x00000 -            0x05FFF = Reserved for BIOS
           0x00600 -            0x00608 = Bootloader data
           0x00700 -            0x007FF = MBR bootstrap after relocation
           0x00800 -            0x009FF = E820 Memory map
           0x00900 -            0x07BFF = Bootloader stack
           0x07C00 -            0x7FFFF = Bootloader code
           0x80000 -            0x9FFFF = Reserved for BIOS
           0xA0000 -            0xB7FFF = VGA video buffer
           0xB8000 -            0xBFFFF = VGA teletype buffer
           0xC0000 -            0xFFFFF = Reserved for BIOS

0x0000000000100000 - 0x0000000000100FFF = PML4 table
0x0000000000101000 - 0x0000000000101FFF = PML3 table
0x0000000000102000 - 0x0000000000102FFF = PML2 table
0x0000000000103000 - 0x0000000000103FFF = PML1 table - maps first 2MB of RAM to do a Long Mode jump

0x0000000000104000 - ...                = Free to use (kernel gets loaded somewhere here)