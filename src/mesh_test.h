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


/* This must not be reduced, as these macros are used to initialize max buffers for holding
 * test results.
 */
#define MAX_ITERATIONS  (100)
#define MAX_DEVICES 	(100)

/* Print connectable adv (proxy advs) counts at this interval */
#define CONNADV_CNT_INT_SEC (600)

/* Messages can travel up to 9 hops */
#define MAX_TTL 	(9)

/* Default number of iterations */
#define DEF_ITERATIONS 	(10)

/* Test results */
struct test_results {
	uint16_t d_id;
	uint16_t addr;
	uint16_t failures;
	int64_t latency[MAX_ITERATIONS];
	uint8_t ttl[MAX_ITERATIONS];
};

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

void bt_mesh_tst_provision(uint16_t addr);
void bt_mesh_tst_common_configure(uint16_t addr);
void bt_mesh_device_setup(const struct bt_mesh_prov *prov, const struct bt_mesh_comp *comp);
void bt_mesh_test_cfg_set(int wait_time);
void bt_mesh_tst_conn_adv_cnt_init(void);
void bt_mesh_tst_conn_adv_cnt_finish(void);

/* Parse DUT list from string like "0,2,5,6" */
void parse_dut_list(const char *dut_str, int *dut_list_out, int *dut_count_out);

/* Check if a device index is in the DUT list */
bool is_dut(int device_idx, int *dut_list, uint32_t dut_count);

void print_common_results(int total_nodes, int max_iterations);

#endif /* ZEPHYR_TESTS_BSIM_BT_MESH_NW_SIM_H_ */
