#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Compile all the applications needed by the bsim tests in these subfolders

# set -x #uncomment this line for debugging
set -ue
: "${ZEPHYR_BASE:?ZEPHYR_BASE must be set to point to the zephyr root directory}"

source ${ZEPHYR_BASE}/tests/bsim/compile.source

SCRIPT_PATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)/$(basename -- "${BASH_SOURCE[0]}")"
APP_DIR="$(dirname "$SCRIPT_PATH")"
APP_DIR="${APP_DIR/${ZEPHYR_BASE}/}"

echo "Compiling applications in ${APP_DIR}"

app=$APP_DIR snippet="bt-ll-sw-split" cmake_args="-DCONFIG_COVERAGE=n" compile

wait_for_background_jobs
