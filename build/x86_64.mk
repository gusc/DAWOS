# Default build

all: bios 

# Build Bios bootable verison

bios:
	make -f bios.mk

# Build EFI bootable version

efi: kernel
	make -f bios.mk
		
# Build GRUB bootable version

grub: kernel
	make -f grub.mk

# Build kernel image

kernel:
	make -f kernel.mk

# Build disk image

#buildimg:
#	dd
