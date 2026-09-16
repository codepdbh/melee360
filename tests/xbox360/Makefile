.SUFFIXES:

ifeq ($(strip $(DEVKITXENON)),)
$(error DEVKITXENON is not set)
endif

include $(DEVKITXENON)/rules

MAKEFILE_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
TARGET := $(abspath $(MAKEFILE_DIR)/../../build-x360/melee360-test)
BUILD := build
SOURCES := ../../src/common ../../src/xbox360 .
INCLUDES := ../../src/common

CFLAGS := -g -O2 -Wall -Wextra $(MACHDEP) $(INCLUDE) \
          -DM360_BUILD_ID=\"$(shell git -C $(MAKEFILE_DIR)/../.. rev-parse --short HEAD 2>/dev/null || echo working-tree)\"
CXXFLAGS := $(CFLAGS)
LDFLAGS = -g $(MACHDEP) -Wl,--gc-sections -Wl,-Map,$(OUTPUT).map
LIBS := -lxenon -lm
LIBDIRS :=

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
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
