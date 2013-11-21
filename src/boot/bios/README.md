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
