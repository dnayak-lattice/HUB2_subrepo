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
# include Makefile.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(KTLIB_DIR)/Makefile.vars
include $(VARS_FILE)

#-----------------------------------------------------------------------------
# Makefile for KtLib
#-----------------------------------------------------------------------------
TGT_DIR  := $(notdir $(KTLIB_DIR))
TGT_NAME := ktlib

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
	$(KT_LIB_DIR)						\
	$(IFACE_DIR)						\
	$(APPS_OS_DIR)

SRC_FILES := $(wildcard $(KT_LIB_DIR)/*.c)
HDR_FILES := $(wildcard $(KT_LIB_DIR)/*.h)
IFACE_FILES := $(wildcard $(IFACE_DIR)/*.h)
APPS_OS_HDR_FILES := $(wildcard $(APPS_OS_DIR)/*.h)

HDR_FILES += $(IFACE_FILES)
HDR_FILES += $(APPS_OS_HDR_FILES)

OBJS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(SRC_FILES)))
DEPS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(SRC_FILES)))

ifeq (debug, $(BUILD_TYPE))
DEBUG_OPTS = -O0 -g -ggdb3
DEFINES += KT_DEBUG
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

build: $(TGT_OUTPUT_DIR) $(DEPS) $(OBJS) $(LIB_FILE)

$(TGT_OUTPUT_DIR):
	$(MKDIR) $(TGT_OUTPUT_DIR)

#-----------------------------------------------------------------------------
# - Generate dependencies (.d) with full paths for each .o
# - Make TGT_OUTPUT_DIR "order-only-prerequisite"
#   To ensure that no .d is rebuilt if its timestamp changes
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: $(KT_LIB_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.o: $(KT_LIB_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_FILE): $(OBJS) $(HDR_FILES)
	$(CC) $(OBJS) -shared -o $@

clean:
	$(RM) $(DEPS) $(OBJS)
	$(RM) $(LIB_FILE)