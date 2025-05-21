/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include "bstests.h"
#include "mesh_test.h"

extern struct bst_test_list *test_network_tst_install(struct bst_test_list *tests);

bst_test_install_t test_installers[] = {
	test_network_tst_install,
	NULL
};

static struct k_thread bsim_mesh_thread;
static K_KERNEL_STACK_DEFINE(bsim_mesh_thread_stack, 4096);

static void bsim_mesh_entry_point(void *unused1, void *unused2, void *unused3)
{
	bst_main();
}

int main(void)
{
	k_thread_create(&bsim_mesh_thread, bsim_mesh_thread_stack,
			K_KERNEL_STACK_SIZEOF(bsim_mesh_thread_stack), bsim_mesh_entry_point, NULL,
			NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);
	k_thread_name_set(&bsim_mesh_thread, "BabbleSim Bluetooth Mesh tests");

	return 0;
}
