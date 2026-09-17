.SUFFIXES:

ifeq ($(strip $(DEVKITXENON)),)
$(error DEVKITXENON is not set)
endif

include $(DEVKITXENON)/rules

MAKEFILE_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
TARGET := $(abspath $(MAKEFILE_DIR)/../../build-x360/melee360-test)
BUILD := build
EMULATOR ?= 0
MELEE_PC := ../../upstream/melee-pc
MELEE_LB := $(MELEE_PC)/src/melee/lb
SOURCES := ../../src/common ../../src/xbox360 . $(MELEE_LB)
INCLUDES := ../../src/common ../../build-x360 $(MELEE_PC)/src \
            $(MELEE_PC)/extern/aurora/include

CFLAGS := -g -O2 -Wall -Wextra $(MACHDEP) $(INCLUDE) \
          -include $(abspath $(MAKEFILE_DIR)/../../src/xbox360/melee_disc_compat.h) \
          -DTARGET_PC=1 -DMELEE_PC=1 \
          -DM360_BUILD_ID=\"$(shell git -C $(MAKEFILE_DIR)/../.. rev-parse --short HEAD 2>/dev/null || echo working-tree)\"
ifeq ($(EMULATOR),1)
CFLAGS += -DM360_EMULATOR_MODE=1 -DM360_DISABLE_VIDEO=1 -DM360_DISABLE_AUDIO=1
endif
CXXFLAGS := $(CFLAGS)
LDFLAGS = -g $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(OUTPUT).map
LIBS := -lxenon -lm -lfat
LIBDIRS :=

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(filter-out $(MELEE_LB),$(SOURCES)),$(notdir $(wildcard $(dir)/*.c))) \
          lbtime.c
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  -I$(CURDIR)/$(BUILD) -I$(LIBXENON_INC)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(LIBXENON_LIB)
export LD := $(CC)

.PHONY: all clean $(BUILD)
all: $(BUILD)
$(BUILD):
	@mkdir -p $(BUILD) $(dir $(TARGET))
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).elf32 $(TARGET).map
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).elf32: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
