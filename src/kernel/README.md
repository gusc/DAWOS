OS Kernel
=========

This is the main OS kernel. It is a bare minimum that is required to make everything work.
Kernel is only responsible for these parts of the workload:
* CPU initialization
* Page and memory management
* IO and interrupt management
* Process scheduling
* System Call processing

Structure
---------

* cpu - CPU initialization and calls
* memory - memory management
* scheduler - process scheduler
* syscall - system call routines
* kmain.* - kernel initialization routines
