# Directory stack PUSH

sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

# Build BBP

# Local variables

OBJ_BBP =
TGT_BBP = ../bin/bbp.img
SRC_DIR_BBP := ../src/$(d)
LF_IMG = -melf_x86_64

# Include sub-rules

dir := $(d)/boot
include ../src/$(dir)/rules.mk

dir := $(d)/loader
include ../src/$(dir)/rules.mk

# Populate global targets and cleanup

TARGETS := $(TARGETS) $(TGT_BBP)
CLEAN := $(CLEAN) $(TGT_BBP) $(OBJ_BBP)

# Local rules

$(TGT_BBP): $(OBJ_BBP)
	$(LD) $(LF_IMG) -T $(SRC_DIR_BBP)/bbp.ld $(OBJ_BBP) -o ../bin/bbp.img

# Directory stack POP

d := $(dirstack_$(sp))
sp := $(basename $(sp))
