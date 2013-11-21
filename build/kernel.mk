# Global tool definitions
AS = nasm
CC = x86_64-pc-elf-gcc
LD = x86_64-pc-elf-ld
OC = x86_64-pc-elf-objcopy

# Global flag definitions
AF_ALL = -felf64 -O0
CF_ALL = -nostartfiles -nostdlib -nodefaultlibs -fno-builtin
LF_ALL = -i

# Target object definition
TARGETS =
# Source directory definitions
SOURCE =
# Cleanup definitions
CLEAN =
# Set source path
VPATH = ../src

# Include child rules
dir := kernel
include ../src/$(dir)/rules.mk

all: $(TARGETS)

# Global autobuild commands

%.s.o: %.asm
	$(AS) $(AF_ALL) -o $@ $<

%.c.o: %.c
	$(CC) $(CF_ALL) -c $< -o $@
	
# Clean up build space

clean:
	rm -f $(CLEAN)