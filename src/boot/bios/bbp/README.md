BIOS Boot Partition
===================

This is the BBP bootcode that get's loaded by MBR. (This directory contaisn 2nd and 3rd stage bootloaders)

Structure
---------

Main entry files:
* boot/boot.asm - main BBP entry code (real mode code)
* boot/ - Real Mode -> Protected Mode -> Long Mode bootstrap code
* kernel/ - Kernel code

Build files:
* bbp.ld - BBP linker script
* config.h - global configuration file (compile-time configuration)
* rules.mk - BBP build rules
