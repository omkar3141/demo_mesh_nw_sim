#!/usr/bin/env bash
# Copyright 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

# Common variables
NODE_COUNT=""
COEFF_FILE=""
MAX_ITERATIONS="10"  # Default value for iterations
DUT_LIST=""         # List of DUTs to test

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
  echo "  -d, --duts LIST      Comma-separated list of DUT indices to test (e.g., \"0,2,5,6\")"
  echo "                       If not specified, all nodes except tester will be tested"
  echo "  -h, --help            Show this help message"
  exit 1
}

# Parse and validate command line arguments
function parse_args() {
  local script_name="$1"
  shift

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
      -d|--duts)
        DUT_LIST="$2"
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

  # Look for coefficient file in current directory
  if [[ -f "$PWD/$COEFF_FILE" ]]; then
    COEFF_FILE_PATH="$PWD/$COEFF_FILE"
  else
    echo "Error: Coefficient file '$COEFF_FILE' not found in directory '$PWD'"
    exit 1
  fi

  # Validate MAX_ITERATIONS
  if [[ -n "$MAX_ITERATIONS" && ! "$MAX_ITERATIONS" =~ ^[0-9]+$ ]]; then
    echo "Error: Iterations must be a positive integer. Got: '$MAX_ITERATIONS'"
    exit 1
  fi

  # Validate DUT_LIST if provided, otherwise generate it
  max_allowed=$((NODE_COUNT - 1))
  if [[ -n "$DUT_LIST" ]]; then
    # Check if DUT_LIST contains only numbers and commas
    if ! [[ "$DUT_LIST" =~ ^[0-9,]+$ ]]; then
      echo "Error: DUT list must contain only numbers and commas. Got: '$DUT_LIST'"
      exit 1
    fi

    # Check if any DUT index is >= NODE_COUNT-1 (last node is tester)
    IFS=',' read -ra DUTS <<< "$DUT_LIST"
    for dut in "${DUTS[@]}"; do
      if [ "$dut" -gt "$max_allowed" ]; then
        echo "Error: DUT index $dut is invalid. Must be between 0 and $max_allowed"
        exit 1
      fi
    done
  else
    # If DUT_LIST is not provided, generate it automatically (all nodes except tester)
    DUT_LIST=$(seq -s, 0 $((max_allowed)))
  fi
}