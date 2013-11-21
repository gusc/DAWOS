#
# BIOS bootloader rules
#

# Directory stack PUSH

sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)



# Directory stack POP

d := $(dirstack_$(sp))
sp := $(basename $(sp))
