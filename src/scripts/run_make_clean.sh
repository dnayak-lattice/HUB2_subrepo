#!/bin/bash
#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

source set_env.sh

# make sure the build directory is cleaned
make -C ${BUILD_DIR} -f Makefile.linux clean 2>&1 > /dev/null
# check if make clean was successful
if [ $? -ne 0 ]; then
    echo "Make clean failed"
    exit 1
fi

echo "Make clean successful"