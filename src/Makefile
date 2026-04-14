#-----------------------------------------------------------------------------
# Copyright (c) 2026 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Makefile Identification
#-----------------------------------------------------------------------------
MKFILE	:= $(realpath $(lastword $(MAKEFILE_LIST)))
MKFILE_REL_PATH := $(dir $(MKFILE))

#-----------------------------------------------------------------------------
# include Makefile.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(MKFILE_REL_PATH)/Makefile.vars
include $(VARS_FILE)

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build_app build_ktlib build_hub clean_app clean_hub clean_ktlib \
clean

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build_hub build_app build_ktlib

build_app: build_hub build_ktlib
	make -C $(APPS_DIR) build

build_hub:
	make -C $(HUB_DIR) build

build_ktlib: build_hub
	make -C $(KTLIB_DIR) build

clean:
	make -C $(APPS_DIR) clean
	make -C $(KT_APP_DIR) clean
	make -C $(HUB_DIR) clean
	make -C $(KTLIB_DIR) clean