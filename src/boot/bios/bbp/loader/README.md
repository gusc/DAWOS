Loader source code
==================

This is the Long Mode OS loader (3rd stage)

Structure
---------

Main entry files:
* main64.c - C entry point that get's called by ../boot/boot.asm

Helper files:
* acpi.* - ACPI table lokup implementation
* ahci.* - AHCI driver
* apic.* - APIC/xAPIC/x2APIC initialization
* common.h - data type definitions
* cpuid.h - inline assembly definition for CPUID instruction
* heap.* - Heap allocator functions
* interrupts.c - interrupt inititialization
* interrupts.asm - interrupt service routines
* interrupts.h - intterupt service routine import in C
* io.h - legacy IO instruction inline definitions
* lib.* - tiny C helper library
* msr.h - Model Specific Register (MSR) instructions inline definitions
* paging.* - Paging functions
* pci.* - PCI operation functions
* debug_print.* - Debug output to text-mode video

Build files:
* loader.ld - Loader linker script
* rules.mk - Loader build rules
