/** @file
 *  @brief Common functionality for Bluetooth Mesh BabbleSim tests.
 */

/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_TESTS_BSIM_BT_MESH_NW_SIM_H_
#define ZEPHYR_TESTS_BSIM_BT_MESH_NW_SIM_H_

#include <zephyr/bluetooth/mesh.h>

#include "bs_types.h"
#include "bstests.h"


#define MODEL_LIST(...) ((const struct bt_mesh_model[]){ __VA_ARGS__ })

#define FAIL(msg, ...)                                                         \
	do {                                                                   \
		bst_result = Failed;                                           \
		bs_trace_error_time_line(msg "\n", ##__VA_ARGS__);             \
	} while (0)

#define PASS()                                                                 \
	do {                                                                   \
		bst_result = Passed;                                           \
		bs_trace_info_time(1, "%s PASSED\n", __func__);                \
	} while (0)

#define ASSERT_TRUE_MSG(cond, fmt, ...)                                        \
	do {                                                                   \
		if (!(cond)) {                                                 \
			bst_result = Failed;                                   \
			bs_trace_error_time_line(                              \
				#cond " is false. " fmt, ##__VA_ARGS__);       \
		}                                                              \
	} while (0)


extern enum bst_result_t bst_result;
void bt_mesh_test_timeout(bs_time_t HW_device_time);
void bt_mesh_device_setup(const struct bt_mesh_prov *prov, const struct bt_mesh_comp *comp);
void bt_mesh_test_cfg_set(int wait_time);

#endif /* ZEPHYR_TESTS_BSIM_BT_MESH_NW_SIM_H_ */
