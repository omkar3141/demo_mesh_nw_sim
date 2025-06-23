# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=8000

function Skip(){
  for i in "${SKIP[@]}" ; do
    if [ "$i" == "$1" ] ; then
      return 0
    fi
  done

  return 1
}

function RunTest(){
  # Set default values
  arg_ch=""
  arg_file=""
  use_nodump=""

  # Parse optional arguments
  while [[ "$1" == arg_ch=* || "$1" == arg_file=* || "$1" == nodump ]]; do
      case "$1" in
          arg_ch=*) arg_ch="${1#arg_ch=}" ;;
          arg_file=*) arg_file="${1#arg_file=}" ;;
          nodump) use_nodump="-nodump" ;;
      esac
      shift
  done

  # Error checking for arg_ch/arg_file
  if [[ "$arg_ch" == "multiatt" ]]; then
    if [[ -z "$arg_file" ]]; then
      echo "Error: When arg_ch is 'multiatt', arg_file must also be provided." >&2
      exit 1
    fi
  elif [[ -n "$arg_ch" || -n "$arg_file" ]]; then
    echo "Error: arg_ch and arg_file must both be set together (with arg_ch='multiatt'), or both be unset." >&2
    exit 1
  fi

  # print source files directory name relative to zephyr base without leading slash and with slashes replaced by underscores
  SCRIPT_PATH="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)/$(basename -- "${BASH_SOURCE[0]}")"
  APP_DIR="$(dirname "$SCRIPT_PATH")"
  APP_DIR="${APP_DIR/${ZEPHYR_BASE}/}"
  APP_DIR="${APP_DIR//\//_}" # Replace slashes with underscores

  # Now $1, $2, ... are the positional arguments
  echo "arg_ch: $arg_ch"
  echo "arg_file: $arg_file"
  echo "Positional args: $@"

  verbosity_level=${verbosity_level:-2}
  extra_devs=${EXTRA_DEVS:-0}

  cd ${BSIM_OUT_PATH}/bin

  idx=0

  s_id=$1
  shift 1

  declare -A testids
  testid=""
  testid_in_order=()

  for arg in $@ ; do
    if [ "$arg" == "--" ]; then
      shift 1
      break
    fi

    if [[ "$arg" == "-"* ]]; then
        testids["${testid}"]+="$arg "
    else
        testid=$arg
        testid_in_order+=($testid)
        testids["${testid}"]=""
    fi

    shift 1
  done

  test_options=$@

  for testid in ${testid_in_order[@]}; do
    if Skip $testid; then
      echo "Skipping $testid (device #$idx)"
      let idx=idx+1
      continue
    fi

    echo "Starting $testid as device #$idx"
    conf=${conf:-prj_conf}

    if [ ${overlay} ]; then
        exe_name=./bs_${BOARD_TS}_${APP_DIR}_${conf}_${overlay}
    else
        exe_name=./bs_${BOARD_TS}_${APP_DIR}_${conf}
    fi

    Execute \
      ${exe_name} \
      -v=${verbosity_level} -s=$s_id -d=$idx -sync_preboot -RealEncryption=1 \
      -testid=$testid ${testids["${testid}"]} ${test_options}
    let idx=idx+1
  done

  count=$(expr $idx + $extra_devs)

  echo "Starting phy with $count devices"

  if [[ "$arg_ch" == "multiatt" ]]; then
    Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=$s_id -D=$count $use_nodump -defmodem=BLE_simple -channel=$arg_ch -argschannel -at=100 -atextra=0 -file=$arg_file
  else
    Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=$s_id -D=$count $use_nodump -argschannel -at=35
  fi

  wait_for_background_jobs
}

function RunTestFlash(){
  s_id=$1
  ext_arg="${s_id} "
  idx=0
  shift 1

  for arg in $@ ; do
    if [ "$arg" == "--" ]; then
      ext_arg+=$@
      break
    fi

    ext_arg+="$arg "

    if [[ "$arg" != "-"* ]]; then
      ext_arg+="-flash=../results/${s_id}/${s_id}_${idx}.bin "
      let idx=idx+1
    fi

    shift 1
  done

  RunTest ${ext_arg}
}
