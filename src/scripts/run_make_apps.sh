#!/bin/bash
#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

source set_env.sh

KT_INTERACTIVE_ELF=${KT_APP_ELF_DIR}/kt_app_interactive.elf
KT_IPC_ELF=${KT_APP_ELF_DIR}/kt_app_ipc.elf

# make sure the apps are built
make -C ${BUILD_DIR} -f Makefile.linux 
# check if make was successful
if [ $? -ne 0 ]; then
    echo "Make apps failed."
    exit 1
fi

echo "Make apps successful."
echo "--------------------------------"
echo "---- ldd interactive app (${KT_INTERACTIVE_ELF}) ----"
ldd ${KT_INTERACTIVE_ELF}
echo "---- ldd ipc app (${KT_IPC_ELF}) ----"
ldd ${KT_IPC_ELF}
echo "--------------------------------"