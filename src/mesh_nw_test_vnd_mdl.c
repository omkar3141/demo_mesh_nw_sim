/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"

#include <zephyr/kernel.h>
#include <bluetooth/mesh/models.h>
#include <zephyr/bluetooth/mesh/proxy.h>
#include "bs_tracing.h"
#include "bs_utils.h"
#include "bsim_args_runner.h"
#include "argparse.h"
#include <vnd_cli.h>
#include <vnd_srv.h>

#define LOG_MODULE_NAME mnt_vnd_mdl
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

extern int max_iterations;
extern int dut_list[MAX_DEVICES];
extern int dut_count;
extern int net_id_counts;

#define WAIT_TIME (max_iterations * MAX_DEVICES * 6)

extern enum bst_result_t bst_result;

extern uint8_t dev_key[16];
extern uint8_t app_key[16];
extern uint8_t app_idx;
extern uint8_t net_key[16];
extern uint8_t net_idx;

extern struct test_results tst_res[MAX_DEVICES];

static struct bt_mesh_prov prov;
static struct bt_mesh_cfg_cli cfg_cli;
static struct bt_mesh_health_srv health_srv;
static struct bt_mesh_model_pub health_pub = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
};

/* Sample message buffer */
static const char set_msg[] =
"Hello World- 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
static const char status_msg[] =
"Response OK- 0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";

/**************************************************************************************************/
/* Vendor model operation callbacks */
static int handle_vendor_set(struct bt_mesh_vendor_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_vendor_set *set,
				struct bt_mesh_vendor_status *rsp);

static int handle_vendor_get(struct bt_mesh_vendor_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_vendor_get *get,
				struct bt_mesh_vendor_status *rsp);

static const struct bt_mesh_vendor_srv_handlers vendor_srv_handlers = {
	.set = handle_vendor_set,
	.get = handle_vendor_get,
};

/* Server model instance */
static struct bt_mesh_vendor_srv vendor_srv = BT_MESH_VENDOR_SRV_INIT(&vendor_srv_handlers);

/* Server set callback */
static int handle_vendor_set(struct bt_mesh_vendor_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_vendor_set *set,
				struct bt_mesh_vendor_status *rsp)
{
	char data[BT_MESH_VENDOR_MSG_MAXLEN_SET + 1] = {0};
	size_t len = set->buf->len;

	/* Safety check for buffer size */
	if (len > BT_MESH_VENDOR_MSG_MAXLEN_SET) {
		len = BT_MESH_VENDOR_MSG_MAXLEN_SET;
	}

	/* Copy data for display */
	if (len > 0) {
		memcpy(data, set->buf->data, len);
		data[len] = '\0'; /* Null-terminate the string */
	}

	LOG_DBG("Received SET message: \"%s\"", data);

	/* Populate the response status message */
	net_buf_simple_reset(rsp->buf);
	net_buf_simple_add_mem(rsp->buf, status_msg, strlen(status_msg));

	return 0; /* Return success to send response immediately */
}

/* Server get callback */
static int handle_vendor_get(struct bt_mesh_vendor_srv *srv,
				struct bt_mesh_msg_ctx *ctx,
				const struct bt_mesh_vendor_get *get,
				struct bt_mesh_vendor_status *rsp)
{
	size_t len = strlen(status_msg);

	/* Check if length parameter is provided and limit response accordingly */
	if (get) {
		len = (get->length < len) ? get->length : len;

		LOG_INF("GET with length: %u", get->length);
	} else {
		LOG_INF("GET without length, sending full response");
	}

	/* Populate the response status message */
	net_buf_simple_reset(rsp->buf);
	net_buf_simple_add_mem(rsp->buf, status_msg, len);

	LOG_INF("Sending STATUS response with length: %zu", len);

	return 0; /* Return success to send response immediately */
}

/**************************************************************************************************/
/* Vendor model client operation callbacks */
/* Forward declaration for the client status handler */
static void handle_vendor_status(struct bt_mesh_vendor_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_vendor_status *status);

/* Client model instance */
static struct bt_mesh_vendor_cli vendor_cli = BT_MESH_VND_CLI_INIT(handle_vendor_status);

/* Client status callback */
static void handle_vendor_status(struct bt_mesh_vendor_cli *cli,
				  struct bt_mesh_msg_ctx *ctx,
				  const struct bt_mesh_vendor_status *status)
{
	char data[BT_MESH_VENDOR_MSG_MAXLEN_STATUS + 1] = {0};
	size_t len = status->buf->len;

	if (len > BT_MESH_VENDOR_MSG_MAXLEN_STATUS) {
		len = BT_MESH_VENDOR_MSG_MAXLEN_STATUS;
	}

	if (len > 0) {
		memcpy(data, status->buf->data, len);
		data[len] = '\0'; /* Null-terminate the string */
	}

	LOG_DBG("Received STATUS: len %d ttl %d", len, ctx->recv_ttl);
}


int vendor_model_send_set(const uint8_t *data, size_t len, struct bt_mesh_msg_ctx *ctx,
			  struct bt_mesh_vendor_status *rsp)
{
	LOG_DBG("Sending SET message: \"%s\" (%d)", (char *)data, len);

	NET_BUF_SIMPLE_DEFINE(temp_buf, BT_MESH_VENDOR_MSG_MAXLEN_SET);
	net_buf_simple_add_mem(&temp_buf, data, len);

	struct bt_mesh_vendor_set set = {
		.buf = &temp_buf
	};

	return bt_mesh_vendor_cli_set(&vendor_cli, ctx, &set, rsp);
}

int vendor_model_send_set_unack(const uint8_t *data, size_t len, struct bt_mesh_msg_ctx *ctx)
{
	LOG_DBG("Sending SET UNACK message: \"%s\" (%d)", (char *)data, len);

	NET_BUF_SIMPLE_DEFINE(temp_buf, BT_MESH_VENDOR_MSG_MAXLEN_SET);
	net_buf_simple_add_mem(&temp_buf, data, len);

	struct bt_mesh_vendor_set set = {
		.buf = &temp_buf
	};

	return bt_mesh_vendor_cli_set_unack(&vendor_cli, ctx, &set);
}

/**************************************************************************************************/

static const struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(1,
		MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
			   BT_MESH_MODEL_CFG_CLI(&cfg_cli),
			   BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub)),
		BT_MESH_MODEL_LIST(
			BT_MESH_MODEL_VND_SRV(&vendor_srv, &vendor_srv_handlers),
			BT_MESH_MODEL_VND_CLI(&vendor_cli))
	),
};

static const struct bt_mesh_comp comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

static void additional_configure(uint16_t addr)
{
	uint8_t status;
	int err;

	/* Bind AppKey to Vendor Server and Client */
	err = bt_mesh_cfg_cli_mod_app_bind_vnd(net_idx, addr, addr, app_idx,
					   BT_MESH_MODEL_ID_VENDOR_SRV, BT_COMP_ID_VENDOR, &status);
	if (err || status) {
		FAIL("Model 0x%04x bind failed (err %d, status %u)", BT_MESH_MODEL_ID_VENDOR_SRV, err,
		     status);
		return;
	}

	err = bt_mesh_cfg_cli_mod_app_bind_vnd(net_idx, addr, addr, app_idx,
					   BT_MESH_MODEL_ID_VENDOR_CLI, BT_COMP_ID_VENDOR, &status);
	if (err || status) {
		FAIL("Model 0x%04x bind failed (err %d, status %u)", BT_MESH_MODEL_ID_VENDOR_CLI, err,
		     status);
		return;
	}
}

static void dev_prov_and_conf(uint16_t addr)
{
	/* Do self provisioning and configuration to keep test bench simple. */
	bt_mesh_tst_provision(addr);
	bt_mesh_tst_common_configure(addr);
	additional_configure(addr);
}

static void test_vnd_node_device_init(void)
{
	bt_mesh_test_cfg_set(WAIT_TIME);
}

static void test_vnd_node_device(void)
{
	bst_result = In_progress;
	LOG_INF("Hello :simid %s nbr %d", bsim_args_get_simid(), bsim_args_get_global_device_nbr());

	bt_mesh_device_setup(&prov, &comp);
	dev_prov_and_conf(bsim_args_get_global_device_nbr() + 1);

	PASS();
}

static void test_vnd_node_tester_init(void)
{
	bt_mesh_test_cfg_set(WAIT_TIME);
}

static void test_vnd_node_tester(void)
{
	int err;
	int64_t t1, t2;

	/* Note: Tester device is instantiated at last in test script, and hence this is also
	 * equal to total number of devices in the network.
	 */
	int total_nodes = bsim_args_get_global_device_nbr() + 1;
	uint16_t tester_addr = total_nodes;
	ASSERT_TRUE_MSG(total_nodes <= MAX_DEVICES, "Edit MAX_DEVICES and recompile");
	LOG_INF("Total Devices : %d", total_nodes);

	/* Setup the device */
	bt_mesh_device_setup(&prov, &comp);

	/* Assign unicast address (simulation ID + 1) and configure it.
	 * Note: device composition has only one element therefore we can just use
	 * (simulation ID + 1) as address.
	 */
	dev_prov_and_conf(tester_addr);

	LOG_INF("Using MAX_ITERATIONS: %d", max_iterations);

	/* Print DUT list if specified */
	if (dut_count > 0) {
		LOG_INF("Testing specific DUTs:");
		for (int i = 0; i < dut_count; i++) {
			LOG_INF("  DUT %d: Device Addr 0x%04x", dut_list[i], dut_list[i] + 1);
		}
	}

	/* For each node, send attention get 10 times and store latency information */
	struct bt_mesh_vendor_status rsp = {0};
	struct bt_mesh_msg_ctx ctx = {0};

	for(int dut = 0; dut < total_nodes; dut++)
	{
		/* Skip if not in DUT list */
		if (!is_dut(dut, dut_list, dut_count)) {
			continue;
		}

		int dut_addr = dut + 1;

		tst_res[dut].d_id = dut;
		tst_res[dut].addr = dut_addr;
		tst_res[dut].failures = 0;

		LOG_INF("Testing latency for Dev: 0x%04x (ID: %d) Tester: 0x%04x", dut_addr,
			 dut, tester_addr);

		for (int i = 0; i < max_iterations; i++) {

			ctx.net_idx = net_idx;
			ctx.app_idx = app_idx;
			ctx.addr = dut_addr;
			ctx.send_ttl = MAX_TTL;
			ctx.send_rel = 0;

			t1 = k_uptime_get();
			err = vendor_model_send_set((const uint8_t *)set_msg, strlen(set_msg),
						    &ctx, &rsp);

			if (err) {
				LOG_ERR("Vendor Set failed (err %d)", err);
				tst_res[dut].failures++;
				k_sleep(K_MSEC(200));
				continue;
			}

			t2 = k_uptime_get();
			tst_res[dut].latency[i] = t2 - t1;

			LOG_INF("Latency: %d", tst_res[dut].latency[i]);

			k_sleep(K_MSEC(1500));
		}

		LOG_INF("Network ID advertisements count %d", net_id_counts);
	}

	print_common_results(total_nodes, max_iterations);

	PASS();

	bs_trace_silent_exit(0);
}

static void test_pre_init(void)
{
	bt_mesh_tst_conn_adv_cnt_init();
}

static void test_terminate(void)
{
	bt_mesh_tst_conn_adv_cnt_finish();
}

/* Parse command line arguments */
static void test_args_parse(int argc, char *argv[])
{
	static char *duts_str;

	bs_args_struct_t args_struct[] = {
		{
			.dest = &max_iterations,
			.type = 'i',
			.name = "{integer}",
			.option = "iterations",
			.descript = "Number of iterations to run for each test"
		},
		{
			.dest = &duts_str,
			.type = 's',
			.name = "{string}",
			.option = "duts",
			.descript = "Comma-separated list of DUT indices to test"
		},
		ARG_TABLE_ENDMARKER
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);

	if (max_iterations < 1 || max_iterations > MAX_ITERATIONS) {
		FAIL("Invalid number of given iterations %d. Max allowed %d", max_iterations,
		     MAX_ITERATIONS);
	}

	dut_count = ARRAY_SIZE(dut_list);
	parse_dut_list(duts_str, dut_list, &dut_count);
}

#define TEST_CASE(role, name, description)                       \
	{                                                        \
		.test_id = #role "_" #name,                      \
		.test_descr = description,                       \
		.test_pre_init_f = test_pre_init,                \
		.test_tick_f = bt_mesh_test_timeout,             \
		.test_args_f = test_args_parse,                  \
		.test_post_init_f = test_##role##_##name##_init, \
		.test_main_f = test_##role##_##name,             \
		.test_delete_f = test_terminate,                 \
	}

static const struct bst_test_instance test_network[] = {
	TEST_CASE(vnd_node, device, "Vendor model nodes in the network"),
	TEST_CASE(vnd_node, tester, "Vendor model tester device"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_vnd_mdl_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_network);
	return tests;
}
