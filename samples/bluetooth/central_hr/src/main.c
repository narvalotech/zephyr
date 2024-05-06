/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

static struct bt_conn *default_conn;

void connect(void)
{
	struct bt_conn_le_create_param *create_param;
	struct bt_le_conn_param *param;
	bt_addr_le_t addr;
	int err;

	bt_addr_le_from_str("44:73:D6:42:3B:05", "(public)", &addr);

	param = BT_LE_CONN_PARAM_DEFAULT;
	create_param = BT_CONN_LE_CREATE_CONN;
	err = bt_conn_le_create(&addr, create_param, param, &default_conn);
	if (err) {
		printk("Create connection failed (err %d)\n", err);
	}
}

volatile bool stat_conn = false;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	stat_conn = true;

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);

		bt_conn_unref(default_conn);
		default_conn = NULL;

		stat_conn = false;

		connect();
		return;
	}

	printk("Connected: %s\n", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	stat_conn = false;

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	connect();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void write_func(struct bt_conn *conn, uint8_t err,
		       struct bt_gatt_write_params *params)
{
	/* (void)memset(params, 0, sizeof(*params)); */
}

static int gatt_write(uint16_t handle, uint16_t offset, void *data, size_t len)
{
	static struct bt_gatt_write_params write_params;

	memset(&write_params, 0, sizeof(write_params));

	write_params.length = len;
	write_params.data = data;
	write_params.handle = handle;
	write_params.offset = offset;
	write_params.func = write_func;

	return bt_gatt_write(default_conn, &write_params);
}

static int gatt_write_wo_rsp(uint16_t handle, void *data, size_t length)
{
	int err;

	if (!default_conn) {
		printk("Not connected\n");
		return -ENOEXEC;
	}

	err = bt_gatt_write_without_response(default_conn, handle,
										 data, length,
										 false);
	if (err) {
		printk("Write failed (err %d)\n", err);
	}

	return err;
}

static void send_cmd(uint16_t opcode)
{
	uint8_t data[2];

	data[0] = opcode >> 8;
	data[1] = opcode & 0xff;

	int err = gatt_write_wo_rsp(0x0010, data, 2);
	__ASSERT(err == 0, "Failed opcode %x (err %d)", opcode, err);
}

static void logi_init(void)
{
	int err;

	uint8_t data[2] = {0x10, 0x00};

	err = gatt_write(0x000c, 0, data, 2);
	if (err) {
		printk("Failed err %d", err);
		return;
	}

	send_cmd(0x0f00);
	send_cmd(0x0e00);
}

int main(void)
{
	int err;
	err = bt_enable(NULL);

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	while (1) {
		printk("Start loop\n");

		while (!stat_conn) {
			k_msleep(2000);
			connect();
		}

		printk("connected\n");

		logi_init();

		printk("init ok\n");

		printk("left\n");
		send_cmd(0x02b6);
		k_msleep(1000);
		printk("stop\n");
		send_cmd(0x02ff);

		printk("Wait...\n");
		for (int i=0; i<2; i++) {
			send_cmd(0x02a5);
			k_msleep(2000);
		}

		printk("right\n");
		send_cmd(0x02a6);
		k_msleep(1000);
		printk("stop\n");
		send_cmd(0x02ff);

		printk("Wait...\n");
		for (int i=0; i<2; i++) {
			send_cmd(0x02a5);
			k_msleep(2000);
		}

		printk("home\n");
		send_cmd(0x1101);
		printk("stop\n");
		send_cmd(0x02ff);

		printk("Wait...\n");
		for (int i=0; i<2; i++) {
			send_cmd(0x02a5);
			k_msleep(2000);
		}

		printk("Disconnect...\n");
		bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		k_msleep(1000);
	}

	return 0;
}
