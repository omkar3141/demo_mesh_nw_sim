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

/* Common test variables */
int max_iterations = DEF_ITERATIONS;

/* DUT list handling */
int dut_list[MAX_DEVICES];
int dut_count = 0;

int net_id_counts;

uint8_t dev_key[16] = { 0xdd };
uint8_t app_key[16] = { 0xaa };
uint8_t app_idx = 0;
uint8_t net_key[16] = { 0xcc };
uint8_t net_idx = 0;

struct test_results tst_res[MAX_DEVICES];


/* Scanner callback function */
static void scan_packet_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
    if (info->adv_type == BT_GAP_ADV_TYPE_ADV_IND) {
        /* FIXME: Parse advertisement data and find out Mesh Proxy Service */

	// since this is simulation
        net_id_counts++;
    }
}

/* Register the scanner callback during initialization */
static struct bt_le_scan_cb scan_cb = {
	.recv = scan_packet_recv,
};

void bt_mesh_tst_provision(uint16_t addr)
{
	int err;

	/* Each device's device key is dddd...<addr> */
	dev_key[0] = addr & 0xFF;
	dev_key[1] = addr >> 8;

	err = bt_mesh_provision(net_key, net_idx, 0, 0, addr, dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}

	LOG_INF("Device provisioned. Addr: 0x%04x", addr);
}

void bt_mesh_tst_common_configure(uint16_t addr)
{
	uint8_t status;
	int err;

	/* Add App Key */
	err = bt_mesh_cfg_cli_app_key_add(net_idx, addr, net_idx, app_idx, app_key,
					  &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}

	/* Change default TTL to suit the maximum network size*/
	uint8_t ttl_status;
	err = bt_mesh_cfg_cli_ttl_set(net_idx, addr, MAX_TTL, &ttl_status);
	if (err) {
		FAIL("Default TTL set failed (err %d, status %u)", err, status);
		return;
	}

	ASSERT_TRUE_MSG(ttl_status == MAX_TTL, "TTL status %u != %u", ttl_status, MAX_TTL);

	bt_mesh_proxy_identity_enable();
	bt_le_scan_cb_register(&scan_cb);
}

/* Track Proxy advertisement counts */
static struct k_work_delayable netid_adv_cnt_work;
static int64_t advcnt_t1, advcnt_t2;

static void net_id_count_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	static int prev = 0;

	LOG_INF("Proxy adv count: %d / sec", (net_id_counts - prev) / CONNADV_CNT_INT_SEC);
	prev = net_id_counts;
	k_work_schedule(&netid_adv_cnt_work, K_SECONDS(CONNADV_CNT_INT_SEC));
}

void bt_mesh_tst_conn_adv_cnt_init(void)
{
	advcnt_t1 = k_uptime_get();
	k_work_init_delayable(&netid_adv_cnt_work, net_id_count_work_handler);
	k_work_schedule(&netid_adv_cnt_work, K_SECONDS(CONNADV_CNT_INT_SEC));
}

void bt_mesh_tst_conn_adv_cnt_finish(void)
{
	advcnt_t2 = k_uptime_get();
	k_work_cancel_delayable(&netid_adv_cnt_work);
	LOG_INF("Total connetable ADVs observed: %d (%d / seconds)",
		net_id_counts, (net_id_counts / ((advcnt_t2 - advcnt_t1)/1000)));
}


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

/* Parse DUT list from string like "0,2,5,6" */
void parse_dut_list(const char *dut_str, int *dut_list_out, int *dut_count_out)
{
	char *str_copy, *token, *saveptr;
	int idx = 0;
	int max_duts = *dut_count_out;

	if (!dut_str || !dut_str[0]) {
		*dut_count_out = 0;
		return;  /* Empty string - will test all nodes */
	}

	/* Make a copy since strtok_r modifies the string */
	str_copy = strdup(dut_str);
	if (!str_copy) {
		FAIL("Failed to allocate memory for DUT list parsing");
		return;
	}

	/* Parse comma-separated values */
	token = strtok_r(str_copy, ",", &saveptr);
	while (token && idx < max_duts) {
		dut_list_out[idx] = atoi(token);
		idx++;
		token = strtok_r(NULL, ",", &saveptr);
	}

	*dut_count_out = idx;
	free(str_copy);
}

/* Check if a device index is in the DUT list */
bool is_dut(int device_idx, int *dut_list, uint32_t dut_count)
{
	/* If no DUTs specified, test all devices */
	if (dut_count == 0 || dut_list == NULL) {
		return true;
	}

	for (uint32_t i = 0; i < dut_count; i++) {
		if (dut_list[i] == device_idx) {
			return true;
		}
	}
	return false;
}

void print_common_results(int total_nodes, int max_iterations)
{
	/* Print average latency */
	LOG_INF("Tester (0x%04x) and each of the devices exchanged %d messages", total_nodes,
		max_iterations);
	LOG_INF("Average round-trip latency for acknowledged messages:");

	for (int dut = 0; dut < total_nodes; dut++) {
		/* Skip if not in DUT list */
		if (!is_dut(dut, dut_list, dut_count)) {
			continue;
		}

		char latency_str[MAX_ITERATIONS * 10 + 100] = {0};
		int64_t avg_latency = 0;
		int offset = 0;

		for (int i = 0; i < max_iterations; i++) {
			avg_latency += tst_res[dut].latency[i];
		}

		avg_latency = avg_latency / max_iterations;

		offset = sprintf(latency_str,
			"Dev %d addr 0x%04x avg latency: %3lld ms failures %d # values: ",
			dut, tst_res[dut].addr, avg_latency, tst_res[dut].failures);

		for (int i = 0; i < max_iterations; i++) {
			offset += sprintf(latency_str + offset, "%lld ", tst_res[dut].latency[i]);
		}

		LOG_INF("%s", latency_str);
	}
}