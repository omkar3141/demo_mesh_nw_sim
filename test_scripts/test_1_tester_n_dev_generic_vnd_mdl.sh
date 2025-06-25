#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

# Examples of use:
# ./test_scripts/test_1_tester_n_dev_generic_vnd_mdl.sh -n 10 -c network1_att_file.coeff -i 10
# ./test_scripts/test_1_tester_n_dev_generic_vnd_mdl.sh -n 24 -c network2_att_file.coeff -i 10
# ./test_scripts/test_1_tester_n_dev_generic_vnd_mdl.sh -n 10 -c network3_att_file.coeff -i 10 -duts "0,2,5,6"

source $(dirname "${BASH_SOURCE[0]}")/../_mesh_test.sh
source $(dirname "${BASH_SOURCE[0]}")/test_common.sh
parse_args "${BASH_SOURCE[0]}" "$@"

# Note: In all test scenarios, tester node must be kept at the end so that tester
# knows the number of devices in the network.
echo "Running test with $NODE_COUNT (devices and tester) nodes."
echo "Using network coefficient file: $COEFF_FILE_PATH"
if [[ -n "$DUT_LIST" ]]; then
  echo "Testing specific DUTs: $DUT_LIST"
fi

node_array=($(printf "vnd_node_device %.0s" $(seq 2 $NODE_COUNT)) "vnd_node_tester")
RunTest nodump arg_ch=multiatt arg_file="$COEFF_FILE_PATH" mesh_nw_sim_test "${node_array[@]}" -- -argstest iterations="$MAX_ITERATIONS" duts="$DUT_LIST"
