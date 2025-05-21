#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../_mesh_test.sh

SCRIPT_PATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)/$(basename -- "${BASH_SOURCE[0]}")"
NW_CONFIG_DIR="$(dirname "$SCRIPT_PATH")/../"

# Note: In all test scenarios, tester node must be kept at the end so that tester
# knows the number of devices in the network.

# Test scenario: network2.png : 10 nodes
RunTest arg_ch=multiatt arg_file=$NW_CONFIG_DIR/network2_att_file.coeff mesh_nw_sim_test \
                node_device \
                node_device \
                node_device \
                node_device \
                node_device \
                node_device \
                node_device \
                node_device \
                node_device \
                node_tester