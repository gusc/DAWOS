#
# BIOS bootloader build script
#

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
# Cleanup definitions
CLEAN =
# Set source path
VPATH = ../src

#
# Include child rules
#
# We need 2 variables - source directory and build directory
# Source directory is for make rules and source files
# Build directory is for object file output
#

# Include MBR build rules
dir := boot/bios/mbr
include ../src/$(dir)/rules.mk

# Include BBP build rules
dir := boot/bios/bbp
include ../src/$(dir)/rules.mk

all: $(TARGETS)
    
test:
	@echo $(OBJ_BOOT)

# Global auto-build commands

%.s.o: %.asm
	$(AS) $(AF_ALL) -o $@ $<

%.c.o: %.c
	$(CC) $(CF_ALL) -c $< -o $@
    
# Clean up build space

clean:
	rm -f $(CLEAN)