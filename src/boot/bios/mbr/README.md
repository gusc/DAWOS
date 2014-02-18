Master Boot Record
==================

This is the MBR bootcode that reads GPT headers and partition array and loads first 512KiB of BIOS Boot Partition bootcode

Structure
---------

* main.asm - main MBR code
* main.inc - data structure definitions and memory location definitions
* rules.mk - build rules for Master Boot Record
