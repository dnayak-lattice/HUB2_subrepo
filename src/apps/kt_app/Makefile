#-----------------------------------------------------------------------------
# Copyright (c) 2026 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Makefile for KT_APP
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Makefile Identification
#-----------------------------------------------------------------------------
MKFILE	:= $(realpath $(lastword $(MAKEFILE_LIST)))

#-----------------------------------------------------------------------------
# include Makefile.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(KT_APP_DIR)/Makefile.vars
include $(VARS_FILE)

#-----------------------------------------------------------------------------
# Makefile for KT_APP
#-----------------------------------------------------------------------------
TGT_DIR  := $(notdir $(KT_APP_DIR))
TGT_NAME := $(TGT_DIR)

#-----------------------------------------------------------------------------
# Build type override
#-----------------------------------------------------------------------------
# Uncomment for release build
#BUILD_TYPE=release

#-----------------------------------------------------------------------------
# Target output directories
#-----------------------------------------------------------------------------
TGT_OUTPUT_DIR:= $(OUTPUT_DIR)/$(TGT_DIR)

# Log directory path (from kt_app.h)
KT_APP_LOG_DIR := $(shell grep "KT_APP_LOG_DIR" $(KT_APP_DIR)/kt_app.h | awk '{print $$3}' | tr -d '"')

# Database directory path (from kt_app.h)
KT_APP_DB_DIR := $(shell grep "KT_APP_DB_DIR" $(KT_APP_DIR)/kt_app.h | awk '{print $$3}' | tr -d '"')

#-----------------------------------------------------------------------------
# Target files - We create two ELF files for the KT_APP
# - One for the interactive mode
# - One for the IPC mode
#-----------------------------------------------------------------------------
KT_INTERACTIVE_ELF_FILE:= $(TGT_OUTPUT_DIR)/$(TGT_NAME)_interactive.elf
KT_IPC_ELF_FILE:= $(TGT_OUTPUT_DIR)/$(TGT_NAME)_ipc.elf

#-----------------------------------------------------------------------------
# Space separated defines for compiler call(s)
# Example: To add "-DMY_OPTION -DMY_OPTION2",
# 		   use "DEFINES += MY_OPTION1 MY_OPTION2"
# Note:    These defines will apply to all compiler invocations
#-----------------------------------------------------------------------------
DEFINES += HUB_OS=$(HUB_OS)

INCLUDES += $(KT_APP_DIR) $(KT_APP_COMMON_DIR) $(KT_APP_INTERACTIVE_DIR) 
INCLUDES += $(KT_APP_IPC_DIR) $(APPS_OS_DIR) $(IFACE_DIR)

# Source files
APPS_OS_SRC_FILES := $(wildcard $(APPS_OS_DIR)/*.c)
COMMON_SRC_FILES := $(wildcard $(KT_APP_COMMON_DIR)/*.c)
INTERACTIVE_SRC_FILES := $(wildcard $(KT_APP_INTERACTIVE_DIR)/*.c)
IPC_SRC_FILES := $(wildcard $(KT_APP_IPC_DIR)/*.c)

# Header files
COMMON_HDR_FILES = $(wildcard $(KT_APP_COMMON_DIR)/*.h)
INTERACTIVE_HDR_FILES = $(wildcard $(KT_APP_INTERACTIVE_DIR)/*.h)
IPC_HDR_FILES = $(wildcard $(KT_APP_IPC_DIR)/*.h)

# All header files
HDR_FILES = $(COMMON_HDR_FILES) $(INTERACTIVE_HDR_FILES) $(IPC_HDR_FILES)

# Oject files
COMMON_OBJS 		= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(COMMON_SRC_FILES)))
INTERACTIVE_OBJS 	= $(COMMON_OBJS) $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(INTERACTIVE_SRC_FILES)))
IPC_OBJS 			= $(COMMON_OBJS) $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(IPC_SRC_FILES)))

# Dependencies
COMMON_DEPS 		= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(COMMON_SRC_FILES)))
INTERACTIVE_DEPS 	= $(COMMON_DEPS) $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(INTERACTIVE_SRC_FILES)))
IPC_DEPS 			= $(COMMON_DEPS) $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(IPC_SRC_FILES)))

# OS object files
OS_OUTPUT_DIR := $(OUTPUT_DIR)/$(notdir $(APPS_OS_DIR))
OS_OBJS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(APPS_OS_SRC_FILES)))
OS_DEPS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(APPS_OS_SRC_FILES)))

ifeq (debug, $(BUILD_TYPE))
DEBUG_OPTS = -O0 -g -ggdb3
DEFINES += KT_DEBUG
else ifeq (release, $(BUILD_TYPE))
DEBUG_OPTS = -O3
else
$(error Unknown build option! Should be debug / release)
endif

# Library paths
KT_LIB_SO_PATH = $(OUTPUT_DIR)/$(notdir $(KT_LIB_DIR))
KT_LIB_SO_FILE = $(basename $(KT_LIB_NAME))
HUB_LIB_SO_PATH = $(OUTPUT_DIR)/$(notdir $(HUB_DIR))
HUB_LIB_SO_FILE = $(basename $(HUB_LIB_NAME))

# Compiler flags
CFLAGS  = $(C_OPTS)
CFLAGS += $(DEBUG_OPTS)

CFLAGS += $(foreach i,$(INCLUDES),-I$(i))
CFLAGS += $(foreach d,$(DEFINES),-D$(d))

CFLAGS += -Wl,-rpath=$(KT_LIB_SO_PATH)
CFLAGS += -Wl,-rpath=$(HUB_LIB_SO_PATH)

# We enable IPC mode by default
CFLAGS += -D KT_APP_IPC_ENABLED

LDFLAGS += -L$(KT_LIB_SO_PATH) -l$(KT_LIB_SO_FILE:lib%=%)
LDFLAGS += -L$(HUB_LIB_SO_PATH) -l$(HUB_LIB_SO_FILE:lib%=%)

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build clean

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build: $(TGT_OUTPUT_DIR) $(KT_APP_LOG_DIR) $(KT_APP_DB_DIR) \
$(COMMON_DEPS) $(INTERACTIVE_DEPS) $(IPC_DEPS) $(COMMON_OBJS) \
$(INTERACTIVE_OBJS) $(IPC_OBJS) $(KT_INTERACTIVE_ELF_FILE) \
$(KT_IPC_ELF_FILE)

$(TGT_OUTPUT_DIR):
	$(MKDIR) $(TGT_OUTPUT_DIR)

$(KT_APP_LOG_DIR):
	$(MKDIR) $(KT_APP_LOG_DIR)

$(KT_APP_DB_DIR):
	$(MKDIR) $(KT_APP_DB_DIR)

#-----------------------------------------------------------------------------
# APPS OS related dependencies and object files
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: $(APPS_OS_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE) $(OS_OBJS) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.o: $(APPS_OS_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

#-----------------------------------------------------------------------------
# KT_APP Common related dependencies and object files
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: $(KT_APP_COMMON_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.o: $(KT_APP_COMMON_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

#-----------------------------------------------------------------------------
# KT_APP Interactive related dependencies and object files
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: $(KT_APP_INTERACTIVE_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.o: $(KT_APP_INTERACTIVE_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(KT_INTERACTIVE_ELF_FILE): $(INTERACTIVE_OBJS) $(HDR_FILES) $(OS_OBJS)
	$(CC) $(CFLAGS) -o $(KT_INTERACTIVE_ELF_FILE) $(INTERACTIVE_OBJS) $(OS_OBJS) $(LDFLAGS)

#-----------------------------------------------------------------------------
# KT_APP IPC related dependencies and object files
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: $(KT_APP_IPC_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.o: $(KT_APP_IPC_DIR)/%.c $(HDR_FILES) $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(KT_IPC_ELF_FILE): $(IPC_OBJS) $(HDR_FILES) $(OS_OBJS)
	$(CC) $(CFLAGS) -o $(KT_IPC_ELF_FILE) $(IPC_OBJS) $(OS_OBJS) $(LDFLAGS)

clean:
	# Do not remove the log and face database location
	$(RM) $(COMMON_DEPS) $(INTERACTIVE_DEPS) $(IPC_DEPS) $(COMMON_OBJS) $(INTERACTIVE_OBJS) \
	$(IPC_OBJS) $(OS_DEPS) $(OS_OBJS)
	$(RM) $(KT_INTERACTIVE_ELF_FILE) $(KT_IPC_ELF_FILE)