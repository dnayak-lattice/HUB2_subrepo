#-----------------------------------------------------------------------------
# Copyright (c) 2026 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Top level Makefile
# This Makefile is used to trigger builds for the following components:
# - Katana Library - src code in: src/KtLib
# - HUB library - src code in: src/hub
# - Katana Application - src code in: src/apps/kt_app
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all clean dist_clean

#-----------------------------------------------------------------------------
# Targets - presently only supports building for Linux
#-----------------------------------------------------------------------------
all:
	make -f Makefile.linux

clean:
	make -f Makefile.linux clean

#-----------------------------------------------------------------------------
# Distribution clean - removed the build output directory
#-----------------------------------------------------------------------------
dist_clean:
	make -f Makefile.linux dist_clean