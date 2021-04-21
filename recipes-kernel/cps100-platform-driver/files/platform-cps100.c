/*
* Platform driver for CPS100 platforms.
*
* platform-cps100.c
*
* (C) Copyright 2014 Hilscher Gesellschaft fuer Systemautomation mbH
* http://www.hilscher.com
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of
* the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/


#define DRIVER_DESC "Platform driver for the CPS100 platform"
#define DRIVER_NAME "platform-cps100"

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/leds.h>

struct io_desc {
	uint8_t		*name;
	uint32_t	portaddr;
	uint32_t	portbit;
	uint32_t	initvalue;
	uint32_t	iotype; /* io=0, in=1, out=2 */
};

struct priv_data_gpio {
	uint8_t		*label;
	uint32_t	gpio_base;
	void		*iodesc;
};

static struct io_desc cps100_iodesc[] = {
	/* name, offset to iobase, portbit, initvalue, iotype */
	{"GPI0", 0x170, 22, 0, 1}, // dfx0
	{"GPI1", 0x270, 23, 0, 1}, // dfx1
	{"GPI2", 0x1c0, 24, 0, 1}, // dfx2
	{"GPI3", 0x1b0, 25, 0, 1}, // dfx3
	{"GPO0", 0x160, 26, 0, 2}, // dfx4
	{"GPO1", 0x150, 27, 0, 2}, // dfx5
	{"GPO2", 0x180, 28, 0, 2}, // dfx6
	{"GPO3", 0x190, 29, 0, 2}, // dfx7
	NULL
};

static struct priv_data_gpio gpio_priv = {
	.label		= "iodev0",
	.gpio_base	= -1,
	.iodesc		= &cps100_iodesc,
};

static struct platform_device cps100_iodev = {
  .name = "gpio-cps100",
  .dev = {
    .platform_data = &gpio_priv,
  },
};

static struct priv_data_gpio nct7904_priv = {
	.label		= "nct7904",
	.gpio_base	= -1,
};


static const unsigned short i2c_nct7904_addr[] = { 0x2d, I2C_CLIENT_END };
static struct i2c_board_info i2c_nct7904_info = {
	.type = "nct7904",
	.platform_data = &nct7904_priv,
};

static struct gpio_led cps100_leds[] = {
  {
    .name = "gpo1:orange:user",
    .default_trigger = "none",
    .active_low = 1,
  },
  {
    .name = "apl:orange:user",
    .default_trigger = "none",
    .active_low = 1,
  },
};

static struct gpio_led_platform_data cps100_leddata = {
  .num_leds	= ARRAY_SIZE(cps100_leds),
  .leds		= cps100_leds,
};

static struct platform_device cps100_leddev = {
  .name	= "leds-gpio",
  .id	= -1,
  .dev	= {
    .platform_data = &cps100_leddata,
  },
};

/**
 * cps100_gpio_init -
 */
static int __init cps100_platform_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);

	/* Register GPIO devices */
	platform_device_register(&cps100_iodev);

	/* Register I2C devices */
	{
		struct i2c_adapter *adapter;
		struct i2c_client *client;

		adapter = i2c_get_adapter(0); // 0 is the bus number
		if (adapter == NULL)
			pr_err("%s: i2c_get_adapter() failed\n", __func__);

		client = i2c_new_probed_device(adapter, &i2c_nct7904_info, i2c_nct7904_addr, NULL);
		if (client == NULL)
			pr_err("%s: i2c_new_probed_device() failed\n", __func__);

		i2c_put_adapter (adapter);
	}

	cps100_leds[0].gpio = nct7904_priv.gpio_base+4; /* GPIOE => offset=4 */
	cps100_leds[1].gpio = nct7904_priv.gpio_base+10;/* GPIO16 => offset=10 */
	platform_device_register(&cps100_leddev);

	return 0;
}
module_init(cps100_platform_init);

/**
 * cps100_gpio_exit -
 */
static void __exit cps100_platform_exit(void)
{
	platform_device_unregister(&cps100_leddev);
	platform_device_unregister(&cps100_iodev);
}
module_exit(cps100_platform_exit);

/* Information about this module */
MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

