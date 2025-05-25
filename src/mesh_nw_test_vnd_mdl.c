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
#include <bs_pc_backchannel.h>
#include <vnd_cli.h>
#include <vnd_srv.h>

#define LOG_MODULE_NAME mnt_vnd_mdl
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define MAX_ITERATIONS (100)
#define MAX_DEVICES (50)
#define NODES_UNDER_TEST {18, 14, 10, 0}

/* Print connectable adv (proxy advs) counts at this interval */
#define CONNADV_CNT_INT_SEC (30)

/* Messages can travel up to 8 hops */
#define MAX_TTL (8)

#define WAIT_TIME (MAX_ITERATIONS * MAX_DEVICES * 2)

/* Backchannel definitions */
#define SYNC_CHAN 0
#define SYNC_MSG_SIZE 1

extern enum bst_result_t bst_result;

struct test_results {
	uint16_t d_id;
	uint16_t addr;
	uint16_t failures;
	int64_t latency[MAX_ITERATIONS];
	uint8_t ttl[MAX_ITERATIONS];
} tst_res[MAX_DEVICES];

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

static int net_id_counts;

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

	LOG_INF("Received STATUS: len %d ttl %d", len, ctx->recv_ttl);
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

	/* Change default TTL to suit the maximum network size*/
	uint8_t ttl_status;
	err = bt_mesh_cfg_cli_ttl_set(net_idx, addr, MAX_TTL, &ttl_status);
	if (err) {
		FAIL("Default TTL set failed (err %d, status %u)", err, status);
		return;
	}

	ASSERT_TRUE_MSG(ttl_status == MAX_TTL, "TTL status %u != %u", ttl_status, MAX_TTL);

	bt_mesh_proxy_identity_enable();
}

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

static void dev_prov_and_conf(uint16_t addr)
{
	/* Do self provisioning and configuration to keep test bench simple. */
	provision(addr);
	common_configure(addr);

	bt_le_scan_cb_register(&scan_cb);
}

/* Track Proxy advertisement counts */
static struct k_work_delayable netid_adv_cnt_work;

static void net_id_count_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	static int prev = 0;

	LOG_INF("Proxy adv count: %d / sec", (net_id_counts - prev) / CONNADV_CNT_INT_SEC);
	prev = net_id_counts;
	k_work_schedule(&netid_adv_cnt_work, K_SECONDS(CONNADV_CNT_INT_SEC));
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

	/* Prepare nodes under test array if NODES_UNDER_TEST is defined, else use all nodes */
#if defined(NODES_UNDER_TEST)
	int nodes_under_test[] = NODES_UNDER_TEST;
	total_nodes = sizeof(nodes_under_test) / sizeof(nodes_under_test[0]);
#else
	/* If NODES_UNDER_TEST is not defined, use all nodes in the network except tester */
	int nodes_under_test[MAX_DEVICES];
	for (int i = 0; i < total_nodes - 1; i++) {
		nodes_under_test[i] = i;
	}
#endif

	LOG_INF("Number of nodes under test: %d", total_nodes);

	/* For each node, send attention get 10 times and store latency information */
	struct bt_mesh_vendor_status rsp = {0};
	struct bt_mesh_msg_ctx ctx = {0};

	for(int d = 0; d < total_nodes; d++)
	{
		int dut_addr = nodes_under_test[d] + 1;
		int dut_id = nodes_under_test[d];

		tst_res[d].d_id = dut_id;
		tst_res[d].addr = dut_addr;
		tst_res[d].failures = 0;

		LOG_INF("Testing latency for Dev: 0x%04x (ID: %d) Tester: 0x%04x", dut_addr,
			 dut_id, tester_addr);

		for (int i = 0; i < MAX_ITERATIONS; i++) {

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
				tst_res[d].failures++;
				k_sleep(K_MSEC(200));
				continue;
			}

			t2 = k_uptime_get();
			tst_res[d].latency[i] = t2 - t1;

			LOG_INF("Latency: %d", tst_res[d].latency[i]);

			k_sleep(K_MSEC(2000));
		}

		LOG_INF("Network ID advertisements count %d", net_id_counts);
	}

	/* Print average latency */
	LOG_INF("Tester (0x%04x) and each of the devices exchanged %d messages", total_nodes,
		MAX_ITERATIONS);
	LOG_INF("Average round-trip latency for acknowledged messages:");

	for (int d = 0; d < total_nodes; d++) {
		char latency_str[MAX_ITERATIONS * 4 + 70] = {0};
		int64_t avg_latency = 0;
		int offset = 0;

		for (int i = 0; i < MAX_ITERATIONS; i++) {
			avg_latency += tst_res[d].latency[i];
		}

		avg_latency = avg_latency / MAX_ITERATIONS;

		offset = sprintf(latency_str, "Dev %d addr 0x%04x avg latency: %3lld ms failures %d # values: ",
			tst_res[d].d_id, tst_res[d].addr, avg_latency, tst_res[d].failures);

		for (int i = 0; i < MAX_ITERATIONS; i++) {
			offset += sprintf(latency_str + offset, "%lld ", tst_res[d].latency[i]);
		}

		LOG_INF("%s", latency_str);
	}


	PASS();

	bs_trace_silent_exit(0);
}

/* For counting connectable ADV traffic */
int64_t advcnt_t1, advcnt_t2;

static void test_back_channel_pre_init(void)
{
	advcnt_t1 = k_uptime_get();
	k_work_init_delayable(&netid_adv_cnt_work, net_id_count_work_handler);
	k_work_schedule(&netid_adv_cnt_work, K_SECONDS(CONNADV_CNT_INT_SEC));
}

static void test_terminate(void)
{
	advcnt_t2 = k_uptime_get();
	k_work_cancel_delayable(&netid_adv_cnt_work);
	LOG_INF("Total connetable ADVs observed: %d (%d / seconds)",
		net_id_counts, (net_id_counts / ((advcnt_t2 - advcnt_t1)/1000)));
}


#define TEST_CASE(role, name, description)                       \
	{                                                        \
		.test_id = #role "_" #name,                      \
		.test_descr = description,                       \
		.test_pre_init_f = test_back_channel_pre_init,   \
		.test_tick_f = bt_mesh_test_timeout,             \
		.test_post_init_f = test_##role##_##name##_init, \
		.test_main_f = test_##role##_##name,             \
		.test_delete_f = test_terminate,                 \
	}

static const struct bst_test_instance test_network[] = {
	TEST_CASE(vnd_node, device, "Nodes in the network"),
	TEST_CASE(vnd_node, tester, "tester device"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_vnd_mdl_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_network);
	return tests;
}
