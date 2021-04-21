/*
* Add platformspecific devices to NIFE200 systems.
*
* platform-nife200.c
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

#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/leds.h>

/**
 *
 */
struct io_desc {
  char *name;
  int portaddr;
  int portbit;
  int initvalue;
  int iotype; /* io=0, in=1, out=2 */
};

/**
 *
 */
static struct io_desc nife200_iodesc[] = {
  /* name, portaddr, portbit, initvalue, iotype */
  {"GPO0", 0xa03, 6, 0, 2},
  {"GPO1", 0xa02, 5, 0, 2},
  {"GPO2", 0xa07, 0, 0, 2},
  {"GPO3", 0xa07, 1, 0, 2},
  {"GPI0", 0xa03, 1, 1, 1},
  {"GPI1", 0xa05, 5, 1, 1},
  {"GPI2", 0xa05, 4, 1, 1},
  {"GPI3", 0xa00, 1, 1, 1},
  NULL
};

static struct platform_device nife200_iodev = {
  .name           = "gpio_nife200",
  .id             = 0, /* => label=iodev0 (id) */
  .dev = {
    .platform_data = nife200_iodesc,
  },
};

/**
 *
 */
static struct io_desc nife200_ioleddesc[] = {
  /* name, portaddr, portbit, initvalue, iotype */
  {"GPO-PR0", 0xa07, 5, 1, 2}, /* led pg1 */
  {"GPO-PR1", 0xa07, 4, 1, 2}, /* led pg2 */
  {"GPO-PR2", 0xa07, 3, 1, 2}, /* led pg3 */
  {"GPO-PR3", 0xa07, 2, 1, 2}, /* led pg4 */
  {"GPO-PR4", 0xa07, 6, 1, 2}, /* led pg0 */
  NULL
};

static struct platform_device nife200_ioleddev = {
  .name           = "gpio_nife200",
  .id             = 100, /* => label=ioleddev0 (id-100) */
  .dev = {
    .platform_data = nife200_ioleddesc,
  },
};

/**
 *
 */
static struct gpio_led nife200_leds[] = {
  {
    .name = "pg1:green:user",
    .default_trigger = "none",
    .gpio = 0,
    .active_low = 1,
  },
  {
    .name = "pg2:orange:user",
    .default_trigger = "none",
    .gpio = 1,
    .active_low = 1,
  },
  {
    .name = "pg3:orange:user",
    .default_trigger = "none",
    .gpio = 2,
    .active_low = 1,
  },
  {
    .name = "pg4:orange:user",
    .default_trigger = "none",
    .gpio = 3,
    .active_low = 1,
  },
  {
    .name = "pg0:orange:user",
    .default_trigger = "none",
    .gpio = 4,
    .active_low = 1,
  },
};

static struct gpio_led_platform_data nife200_leddata = {
  .num_leds  = ARRAY_SIZE(nife200_leds),
  .leds    = nife200_leds
};

static struct platform_device nife200_leddev = {
  .name    = "leds-gpio",
  .id    = -1,
  .dev    = {
    .platform_data  = &nife200_leddata ,
  },
};

/**
 *
 */
static int nife200_platform_init(void)
{
  int rc = 0;

  /* TODO: error handling */
  rc = platform_device_register(&nife200_iodev);
  rc = platform_device_register(&nife200_ioleddev);
  rc = platform_device_register(&nife200_leddev);

err:
  return rc;
}
module_init(nife200_platform_init);

/**
 *
 */
static void nife200_platform_exit(void)
{
  platform_device_unregister(&nife200_leddev);
  platform_device_unregister(&nife200_ioleddev);
  platform_device_unregister(&nife200_iodev);
}
module_exit(nife200_platform_exit);

/* Information about this module */
MODULE_DESCRIPTION("NIFE200 platform devices");
MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_LICENSE("GPL");

