#!/bin/bash

source set_env.sh

source ${KTENV_DIR}/bin/activate

cd ${SRC_DIR}/apps/kt_viz

python app.py