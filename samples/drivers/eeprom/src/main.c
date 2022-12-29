/*
 * Copyright (c) 2021 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, 4);

#define EEPROM_SAMPLE_OFFSET 0
#define EEPROM_SAMPLE_MAGIC  0xEE9703

struct perisistant_values {
	uint32_t magic;
	uint32_t boot_count;
};

/*
 * Get a device structure from a devicetree node with alias eeprom-0
 */
static const struct device *get_eeprom_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(eeprom_0));

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found EEPROM device \"%s\"\n", dev->name);
	return dev;
}

static uint8_t rxbuf[4096];

void main(void)
{
	const struct device *eeprom = get_eeprom_device();
	size_t eeprom_size;
	int rc;

	if (eeprom == NULL) {
		return;
	}

	eeprom_size = eeprom_get_size(eeprom);
	printk("Using eeprom with size of: %zu.\n", eeprom_size);

	for (int a=0; a<512; a++) {
		rc = eeprom_read(eeprom, a, &rxbuf, 1);
		if (rc < 0) {
			printk("Error: Couldn't read eeprom: err: %d.\n", rc);
			return;
		} else {
			LOG_DBG("read[%u]\t %x", a, rxbuf[0]);
		}
	}

	/* Sequential readout */
	uint16_t len = 4096;

	/* Driver prevents us to read across boundary (and test the overflow on the
	 * other side), too bad.
	 */
	rc = eeprom_read(eeprom, 0, rxbuf, len);
	if (rc < 0) {
		printk("Error: Couldn't read eeprom: err: %d.\n", rc);
		return;
	} else {
		LOG_HEXDUMP_DBG(rxbuf, 2048, "I2C RX:");
		LOG_HEXDUMP_DBG(&rxbuf[2048], 2048, "I2C RX:");
	}
}
