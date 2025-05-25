# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_mesh_nw_sim)

include_directories(
	${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common
)

target_sources(app PRIVATE
  src/main.c
  src/mesh_test.c
  src/mesh_nw_test.c
  src/mesh_nw_test_vnd_mdl.c
  vnd_mdl/src/vnd_cli.c
  vnd_mdl/src/vnd_srv.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/samples/bluetooth/mesh/common
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/mesh
  vnd_mdl/include
)

zephyr_include_directories(
  ${BSIM_COMPONENTS_PATH}/libUtilv1/src/
  ${BSIM_COMPONENTS_PATH}/libPhyComv1/src/
)
