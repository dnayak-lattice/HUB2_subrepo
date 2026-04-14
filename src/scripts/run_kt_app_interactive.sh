#!/bin/bash
#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

source set_env.sh

ELF_FILE=${KT_APP_ELF_DIR}/kt_app_interactive.elf

# make sure the ELF file is built
make -C ${BUILD_DIR} -f Makefile.linux 2>&1 > /dev/null
# check if make was successful
if [ $? -ne 0 ]; then
    echo "Make failed"
    exit 1
fi

# Print the command to be executed
echo "Executing: ${ELF_FILE}"
${ELF_FILE}    