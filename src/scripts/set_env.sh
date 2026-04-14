#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#set -x 

# Get the PROJECT base directory referencing the relative path of CWD
# scripts/ is in <PROJECT_BASE_DIR>/src/scripts 
PROJECT_BASE_DIR=$(dirname $(dirname $(pwd)))

BUILD_DIR=${PROJECT_BASE_DIR}/build
SRC_DIR=${PROJECT_BASE_DIR}/src
OUTPUT_DIR=${BUILD_DIR}/output
APPS_DIR=${OUTPUT_DIR}/apps
KT_APP_ELF_DIR=${OUTPUT_DIR}/kt_app
SCRIPTS_DIR=${PROJECT_BASE_DIR}/src/scripts

# Katana environment directory - hardcoded for now
ENV_ROOT_DIR="/home/lattice/Katana_1.0.0"
KTENV_DIR="${ENV_ROOT_DIR}/ktenv"

export PROJECT_BASE_DIR
export BUILD_DIR
export OUTPUT_DIR
export APPS_DIR
export KT_APP_ELF_DIR
export SCRIPTS_DIR

export ENV_ROOT_DIR
export KTENV_DIR