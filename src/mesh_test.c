/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mesh_test.h"

#include <zephyr/kernel.h>
#include "bs_tracing.h"
#include "time_machine.h"

#define LOG_MODULE_NAME mesh_test
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

void bt_mesh_device_setup(const struct bt_mesh_prov *prov, const struct bt_mesh_comp *comp)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_mesh_init(prov, comp);
	if (err) {
		FAIL("Initializing mesh failed (err %d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		LOG_INF("Loading stored settings");
		if (IS_ENABLED(CONFIG_BT_MESH_USES_MBEDTLS_PSA)) {
			settings_load_subtree("itsemul");
		}
		settings_load_subtree("bt");
	}

	LOG_INF("Mesh initialized");
}

void bt_mesh_test_timeout(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test timeout (not passed after %i seconds)",
		     HW_device_time / USEC_PER_SEC);
	}

	bs_trace_silent_exit(0);
}

void bt_mesh_test_cfg_set(int wait_time)
{
	bst_ticker_set_next_tick_absolute(wait_time * USEC_PER_SEC);
	bst_result = In_progress;

	/* Ensure those test devices will not drift more than
	 * 100ms for each other in emulated time
	 */
	tm_set_phy_max_resync_offset(100000);
}
