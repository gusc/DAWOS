# Directory stack PUSH

sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Build boot

# Local variables

TGT_BOOT := $(d)/boot.o
OBJ_BOOT := $(d)/boot.s.o $(d)/bios.s.o $(d)/main16.o $(d)/main32.o 
SRC_DIR_BOOT := ../src/$(d)
OBJ_DIR_BOOT := $(d)
CF_32 = -m32 -march=i686 -Wno-attributes
OF_32 = -I elf32-i386 -O elf64-x86-64

# Populate global target and clean
	
CLEAN := $(CLEAN) $(OBJ_BOOT) $(d)/main16.c32.o $(d)/main32.c32.o
OBJ_BBP := $(OBJ_BBP) $(TGT_BOOT)

# Local rules

$(TGT_BOOT): $(OBJ_BOOT)
	$(LD) $(LF_ALL) -T $(SRC_DIR_BOOT)/boot.ld $(OBJ_BOOT) -o $(TGT_BOOT)

$(d)/main16.o:
	$(CC) $(CF_32) -c $(SRC_DIR_BOOT)/main16.c -o $(OBJ_DIR_BOOT)/main16.c32.o
	$(OC) $(OF_32) $(OBJ_DIR_BOOT)/main16.c32.o $(OBJ_DIR_BOOT)/main16.o

$(d)/main32.o:
	$(CC) $(CF_32) -c $(SRC_DIR_BOOT)/main32.c -o $(OBJ_DIR_BOOT)/main32.c32.o
	$(OC) $(OF_32) $(OBJ_DIR_BOOT)/main32.c32.o $(OBJ_DIR_BOOT)/main32.o	

# Directory stack POP

d := $(dirstack_$(sp))
sp := $(basename $(sp))
