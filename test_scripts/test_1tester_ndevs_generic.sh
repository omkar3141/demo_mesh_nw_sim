#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../_mesh_test.sh

SCRIPT_PATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)/$(basename -- "${BASH_SOURCE[0]}")"
NW_CONFIG_DIR="$(dirname "$SCRIPT_PATH")/../"
NODE_COUNT=""
COEFF_FILE=""
MAX_ITERATIONS="10"  # Default value for iterations

# Usage information
function show_usage() {
  echo "Usage: $0 [REQUIRED OPTIONS]"
  echo "Required Options:"
  echo "  -n, --nodes NUM       Number of nodes in the network (2-200)"
  echo "                       (Note: The tester node is included in this count)"
  echo "                       (e.g., for 10 nodes, specify -n 10, which includes 9 device nodes and 1 tester node)"
  echo "  -c, --coeff FILE      Network coefficient file"
  echo "Other Options:"
  echo "  -i, --iterations NUM  Number of iterations for the test (default: 10)"
  echo "  -h, --help            Show this help message"
  exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -n|--nodes)
      NODE_COUNT="$2"
      shift 2
      ;;
    -c|--coeff)
      COEFF_FILE="$2"
      shift 2
      ;;
    -i|--iterations)
      MAX_ITERATIONS="$2"
      shift 2
      ;;
    -h|--help)
      show_usage
      ;;
    *)
      echo "Error: Unknown option: $1"
      show_usage
      ;;
  esac
done

if [[ -z "$NODE_COUNT" || -z "$COEFF_FILE" ]]; then
  echo "Error: Both node count (-n) and coefficient file (-c) are required."
  show_usage
fi

if ! [[ "$NODE_COUNT" =~ ^[0-9]+$ ]] || [ "$NODE_COUNT" -lt 2 ] || [ "$NODE_COUNT" -gt 200 ]; then
  echo "Error: Node count must be a positive integer between 2 and 200. Got: '$NODE_COUNT'"
  exit 1
fi

if [[ -f "$NW_CONFIG_DIR/$COEFF_FILE" ]]; then
  COEFF_FILE_PATH="$NW_CONFIG_DIR/$COEFF_FILE"
else
  COEFF_FILE_PATH="$COEFF_FILE"
fi

if [[ ! -f "$COEFF_FILE_PATH" ]]; then
  echo "Error: Coefficient file '$COEFF_FILE_PATH' not found."
  exit 1
fi

# Validate MAX_ITERATIONS
if [[ -n "$MAX_ITERATIONS" && ! "$MAX_ITERATIONS" =~ ^[0-9]+$ ]]; then
  echo "Error: Iterations must be a positive integer. Got: '$MAX_ITERATIONS'"
  exit 1
fi

# Note: In all test scenarios, tester node must be kept at the end so that tester
# knows the number of devices in the network.

# Run the test
echo "Running test with $NODE_COUNT (devices and tester) nodes."
echo "Using network coefficient file: $COEFF_FILE_PATH"
node_array=($(printf "node_device %.0s" $(seq 2 $NODE_COUNT)) "node_tester")
RunTest arg_ch=multiatt arg_file="$COEFF_FILE_PATH" mesh_nw_sim_test "${node_array[@]}" -- -argstest iterations="$MAX_ITERATIONS"
