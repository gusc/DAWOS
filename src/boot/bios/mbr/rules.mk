#
# Master Boot Record build rules
#

# Directory stack PUSH
sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)
# End PUSH

# Local variables

TGT_MBR := ../bin/mbr.img
SRC_DIR_MBR := ../src/$(d)
AF_BIN = -fbin

# Populate global targets and cleanup

TARGETS := $(TARGETS) $(TGT_MBR)
CLEAN := $(CLEAN) $(TGT_MBR)

# Local rules

$(TGT_MBR):
	$(AS) $(AF_BIN) -o $(TGT_MBR) $(SRC_DIR_MBR)/main.asm

# Directory stack POP
d := $(dirstack_$(sp))
sp := $(basename $(sp))
# End POP
