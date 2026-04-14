#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Makefile Identification
#-----------------------------------------------------------------------------
MKFILE	:= $(realpath $(lastword $(MAKEFILE_LIST)))

#-----------------------------------------------------------------------------
# Makefile for HUB library
#-----------------------------------------------------------------------------
TGT_DIR  := $(notdir $(HUB_DIR))
TGT_NAME := hub

#-----------------------------------------------------------------------------
# Build type override
#-----------------------------------------------------------------------------
# Uncomment for release build
#BUILD_TYPE=release

#-----------------------------------------------------------------------------
# target output dirs
#-----------------------------------------------------------------------------
TGT_OUTPUT_DIR:= $(OUTPUT_DIR)/$(TGT_DIR)

#-----------------------------------------------------------------------------
# target definitions
#-----------------------------------------------------------------------------
LIB_FILE         := $(TGT_OUTPUT_DIR)/lib$(TGT_NAME).so

#-----------------------------------------------------------------------------
# Space separated defines for compiler call(s)
# Example: To add "-DMY_OPTION -DMY_OPTION2",
# 		   use "DEFINES += MY_OPTION1 MY_OPTION2"
# Note:    These defines will apply to all compiler invocations
#-----------------------------------------------------------------------------
INCLUDES +=					            \
	$(HUB_DIR)			            	\
	$(IFACE_DIR)						\
	$(APPS_OS_DIR)

SRC_FILES := $(wildcard $(HUB_DIR)/*.c)

HUB_HDR_FILES := $(wildcard $(HUB_DIR)/*.h)
HUB_IFACE_FILES := $(wildcard $(IFACE_DIR)/*.h)
APPS_OS_FILES := $(wildcard $(APPS_OS_DIR)/*.h)

HUB_HDR_FILES += $(HUB_IFACE_FILES)
HUB_HDR_FILES += $(APPS_OS_FILES)

OBJS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(SRC_FILES)))
DEPS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(SRC_FILES)))

ifeq (debug, $(BUILD_TYPE))
DEBUG_OPTS = -O0 -g -ggdb3
else ifeq (release, $(BUILD_TYPE))
DEBUG_OPTS = -O3
else
$(error Unknown build option! Should be debug / release)
endif

CFLAGS  = $(C_OPTS)
CFLAGS += $(DEBUG_OPTS)

CFLAGS += $(foreach i,$(INCLUDES),-I$(i))
CFLAGS += $(foreach d,$(DEFINES),-D$(d))

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build clean

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build: $(TGT_OUTPUT_DIR) $(DEPS) $(LIB_FILE)

$(TGT_OUTPUT_DIR):
	$(MKDIR) $(TGT_OUTPUT_DIR)

$(TGT_OUTPUT_DIR)/%.o: $(HUB_DIR)/%.c $(HUB_HDR_FILES) $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

#-----------------------------------------------------------------------------
# - Generate dependencies (.d) with full paths for each .o
# - Make TGT_OUTPUT_DIR "order-only-prerequisite"
#   To ensure that no .d is rebuilt if its timestamp changes
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: $(HUB_DIR)/%.c $(HUB_HDR_FILES) $(MKFILE) $(VARS_FILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(LIB_FILE): $(OBJS) $(HUB_HDR_FILES)
	$(CC) $(OBJS) -shared -o $@

clean:
	$(RM) $(DEPS) $(OBJS)
	$(RM) $(LIB_FILE)