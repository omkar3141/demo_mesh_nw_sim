/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"

#include <zephyr/kernel.h>
#include "bs_tracing.h"
#include "bs_utils.h"
#include "bsim_args_runner.h"

#define LOG_MODULE_NAME mesh_nw_test
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define MAX_ITERATIONS (10)
#define MAX_DEVICES (100)

/* Messages can travel up to 9 hops */
#define MAX_TTL (9)

#define WAIT_TIME (MAX_ITERATIONS * MAX_DEVICES * 2)

extern enum bst_result_t bst_result;

static uint8_t dev_key[16] = { 0xdd };
static uint8_t app_key[16] = { 0xaa };
static uint8_t app_idx = 0;
static uint8_t net_key[16] = { 0xcc };
static uint8_t net_idx = 0;

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_health_srv health_srv;
static struct bt_mesh_model_pub health_pub = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
};

static void health_attention_status(struct bt_mesh_health_cli *cli,
				    uint16_t addr, uint8_t attention)
{
	LOG_INF("Health Attention Status from 0x%04x: %u", addr, attention);
}

struct bt_mesh_health_cli health_cli = {
	.current_status = NULL,
	.fault_status = NULL,
	.attention_status = health_attention_status,
	.period_status = NULL,
};

static const struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(1,
		MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
			   BT_MESH_MODEL_CFG_CLI(&cfg_cli),
			   BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
			   BT_MESH_MODEL_HEALTH_CLI(&health_cli)),
		BT_MESH_MODEL_NONE),
};

const struct bt_mesh_comp comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

static void provision(uint16_t addr)
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

static void common_configure(uint16_t addr)
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

	/* Bind it to Health Server and Client */
	err = bt_mesh_cfg_cli_mod_app_bind(net_idx, addr, addr, app_idx,
					   BT_MESH_MODEL_ID_HEALTH_SRV, &status);
	if (err || status) {
		FAIL("Model 0x%04x bind failed (err %d, status %u)", BT_MESH_MODEL_ID_HEALTH_SRV, err,
		     status);
		return;
	}

	err = bt_mesh_cfg_cli_mod_app_bind(net_idx, addr, addr, app_idx,
					   BT_MESH_MODEL_ID_HEALTH_CLI, &status);
	if (err || status) {
		FAIL("Model 0x%04x bind failed (err %d, status %u)", BT_MESH_MODEL_ID_HEALTH_SRV, err,
		     status);
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
}


static void dev_prov_and_conf(uint16_t addr)
{
	/* Do self provisioning and configuration to keep test bench simple. */
	provision(addr);
	common_configure(addr);
}

static void test_node_device_init(void)
{
	bt_mesh_test_cfg_set(WAIT_TIME);
}

static void test_node_device(void)
{
	bst_result = In_progress;
	LOG_INF("Hello :simid %s nbr %d", bsim_args_get_simid(), bsim_args_get_global_device_nbr());

	bt_mesh_device_setup(&prov, &comp);
	dev_prov_and_conf(bsim_args_get_global_device_nbr() + 1);

	PASS();
}

static void test_node_tester_init(void)
{
	bt_mesh_test_cfg_set(WAIT_TIME);
}

static void test_node_tester(void)
{
	int err;
	int64_t t1, t2;

	/* Note: Tester device is instantiated at last in test script, and hence this is also
	 * equal to total number of devices in the network.
	 */
	int total_nodes = bsim_args_get_global_device_nbr() + 1;
	ASSERT_TRUE_MSG(total_nodes <= MAX_DEVICES, "Edit MAX_DEVICES and recompile");
	LOG_INF("Total Devices : %d", total_nodes);

	/* Setup the device */
	bt_mesh_device_setup(&prov, &comp);

	/* Assign unicast address (simulation ID + 1) and configure it.
	 * Note: device composition has only one element therefore we can just use
	 * (simulation ID + 1) as address.
	 */
	dev_prov_and_conf(total_nodes);

	/* For each node, send attention get 10 times and store latency information */
	int64_t latency[MAX_DEVICES][MAX_ITERATIONS] = {0};
	int failures[MAX_DEVICES] = {0};

	for(int dut = 0; dut < total_nodes; dut++)
	{
		int addr = dut + 1;
		LOG_INF("Testing latency for Device Addr: 0x%04x", addr);

		for (int i = 0; i < MAX_ITERATIONS; i++) {

			struct bt_mesh_msg_ctx ctx = {0};
			uint8_t attention;

			ctx.net_idx = net_idx;
			ctx.app_idx = app_idx;
			ctx.addr = addr;
			ctx.send_ttl = MAX_TTL;
			ctx.send_rel = 0;

			t1 = k_uptime_get();
			err = bt_mesh_health_cli_attention_get(&health_cli, &ctx, &attention);

			if (err) {
				LOG_ERR("Health Attention Get failed (err %d)", err);
				failures[dut]++;
				k_sleep(K_MSEC(200));
				continue;
			}

			t2 = k_uptime_get();
			latency[dut][i] = t2 - t1;

			LOG_INF("Latency: %d", latency[dut][i]);

			k_sleep(K_MSEC(1200));
		}
	}

	/* Print average latency */
	LOG_INF("Tester (0x%04x) and each of the devices exchanged %d messages", total_nodes,
		MAX_ITERATIONS);
	LOG_INF("Average round-trip latency for acknowledged messages:");

	for (int d = 0; d < total_nodes; d++) {
		int64_t avg_latency = 0;
		char latency_str[MAX_ITERATIONS * 4 + 50] = {0};
		int offset = 0;

		for (int i = 0; i < MAX_ITERATIONS; i++) {
			avg_latency += latency[d][i];
		}

		avg_latency = avg_latency / MAX_ITERATIONS;

		offset = sprintf(latency_str, "Dev %d addr 0x%04x avg latency: %3lld ms failures %d # values: ",
			d, d + 1, avg_latency, failures[d]);

		for (int i = 0; i < MAX_ITERATIONS; i++) {
			offset += sprintf(latency_str + offset, "%lld ", latency[d][i]);
		}

		LOG_INF("%s", latency_str);
	}

	PASS();

	bs_trace_silent_exit(0);
}

#define TEST_CASE(role, name, description)                       \
	{                                                        \
		.test_id = #role "_" #name,                      \
		.test_descr = description,                       \
		.test_tick_f = bt_mesh_test_timeout,             \
		.test_post_init_f = test_##role##_##name##_init, \
		.test_main_f = test_##role##_##name,             \
	}

static const struct bst_test_instance test_network[] = {
	TEST_CASE(node, device, "Nodes in the network"),
	TEST_CASE(node, tester, "tester device"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_network_tst_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_network);
	return tests;
}
