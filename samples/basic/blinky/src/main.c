#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <string.h>
#include <drivers/display.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

struct display_buffer_descriptor disp_desc = {
	/** Data buffer size in bytes */
	.buf_size = 128 * 16,
	/** Data buffer row width in pixels */
	.width = 128,
	/** Data buffer column height in pixels */
	.height = 128,
	/** Number of pixels between consecutive rows in the data buffer */
	.pitch = 128,
};

uint8_t buf[128*16] = {0};

void main(void)
{
	const struct device *display_dev;
	display_dev = device_get_binding("LS0XX");

	uint8_t data = 0xa0;

	display_blanking_off(display_dev);

	while(1) {
		if(data)
			data = 0;
		else
			data = 0xa0;

		memset(buf, data, sizeof(buf));

		printk("Write\n");
		display_write(display_dev, 0, 0, &disp_desc, buf);
		k_msleep(500);
	}
}
