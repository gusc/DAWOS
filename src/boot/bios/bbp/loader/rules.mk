# Directory stack PUSH

sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Build Kernel Loader

# Local variables

TGT_LDR := $(d)/loader.o
OBJ_LDR := $(d)/lib.c.o $(d)/interrupts.s.o $(d)/interrupts.c.o \
	$(d)/debug_print.c.o $(d)/pic.c.o \
	$(d)/paging.c.o $(d)/heap.c.o $(d)/memory.c.o \
	$(d)/pci.c.o $(d)/ahci.c.o $(d)/main64.c.o
SRC_DIR_LDR := ../src/$(d)

# Populate global target and clean
	
CLEAN := $(CLEAN) $(OBJ_LDR)
OBJ_BBP := $(OBJ_BBP) $(TGT_LDR)

# Local rules

$(TGT_LDR): $(OBJ_LDR)
	$(LD) $(LF_ALL) -T $(SRC_DIR_LDR)/loader.ld $(OBJ_LDR) -o $(TGT_LDR)

# Directory stack POP

d := $(dirstack_$(sp))
sp := $(basename $(sp))
