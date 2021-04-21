/*
* GPIO driver for NIFE200 and CPS100 platforms.
*
* gpio-nife200.c
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

#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/ioport.h>

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
 * Private data block
 */
struct priv_data {
  char label[32];
  spinlock_t lock;
  struct io_desc *iodesc;
  struct gpio_chip chip;
};

/**
 * nife200_gpio_get - Read the value of the specified GPIO.
 */
static int nife200_gpio_get(struct gpio_chip *chip, unsigned off)
{
  struct priv_data *priv = container_of(chip, struct priv_data, chip);
  unsigned long portval;

  portval = inl((priv->iodesc)[off].portaddr);

  return (portval & (1 << (priv->iodesc)[off].portbit)) ? 1 : 0;
}

/**
 * nife200_gpio_set - Set a new value of the specified GPIO.
 */
static void nife200_gpio_set(struct gpio_chip *chip, unsigned off, int val)
{
  struct priv_data *priv = container_of(chip, struct priv_data, chip);
  unsigned long portval;

  spin_lock(&priv->lock);

  portval = inl((priv->iodesc)[off].portaddr);

  if (val) {
    portval |= (1 << (priv->iodesc)[off].portbit);
  }
  else {
    portval &= ~(1 << (priv->iodesc)[off].portbit);
  }

  outl(portval, (priv->iodesc)[off].portaddr);

  spin_unlock(&priv->lock);
}

/**
 * nife200_gpio_dir_in - Set the direction of the specified GPIO as input.
 */
static int nife200_gpio_dir_in(struct gpio_chip *chip, unsigned off)
{
  struct priv_data *priv = container_of(chip, struct priv_data, chip);

  if ((priv->iodesc)[off].iotype == 2) { /* 2=output */
    pr_err("Error: This GPIO is a dedicated output!\n");
    return -1;
  }

  /* TODO: Add code to reconfigure the GPIO as input. */

  return 0;
}

/**
 * nife200_gpio_dir_out - Set the direction of the specified GPIO as output.
 */
static int nife200_gpio_dir_out(struct gpio_chip *chip, unsigned off, int val)
{
  struct priv_data *priv = container_of(chip, struct priv_data, chip);

  if ((priv->iodesc)[off].iotype == 1) { /* 1=input */
    pr_err("Error: This GPIO is a dedicated input!\n");
    return -1;
  }

  /* TODO: Add code to reconfigure the GPIO as output. */

  nife200_gpio_set(chip, off, val);

  return 0;
}

/**
 * nife200_gpio_probe - Probe method for the GPIO devices.
 */
static int nife200_gpio_probe(struct platform_device *pdev)
{
  int rc = 0, n, ngpio, base;
  struct priv_data *priv = NULL;
  struct io_desc *iodesc;

  /* Add per-device initialization code here */

  /* Allocate zero initialized memory for private data */
  priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
  if (priv == NULL) {
    pr_err("%s: devm_kzalloc() failed\n", __func__);
    return -ENOMEM;
  }

  /* Link the gpio descriptions for private use */
  priv->iodesc = dev_get_platdata(&pdev->dev);

  /* Set up instance specific items */
  if (pdev->id < 100) {
    base = -1; /* dynamic base address */
    snprintf(priv->label, sizeof(priv->label), "iodev%d", pdev->id);
  }
  else if (pdev->id >= 100) {
    base = pdev->id-100; /* leds have fixed base addresses */
    snprintf(priv->label, sizeof(priv->label), "ioleddev%d", pdev->id-100);
  }

  /* Count the number of gpios */
  iodesc = priv->iodesc;
  while (*(char*)iodesc != '\0') iodesc++;
  ngpio = (iodesc - priv->iodesc);

  /* Configure the chip */
  priv->chip.label = priv->label;
  priv->chip.base  = base;
  priv->chip.ngpio = ngpio;
  priv->chip.owner = THIS_MODULE;
  priv->chip.get = nife200_gpio_get;
  priv->chip.set = nife200_gpio_set;
  priv->chip.direction_input = nife200_gpio_dir_in;
  priv->chip.direction_output = nife200_gpio_dir_out;
  priv->chip.can_sleep = false;

  spin_lock_init(&priv->lock);

  platform_set_drvdata(pdev, priv);

  rc = gpiochip_add(&priv->chip);
  if (rc) {
    pr_err("%s: gpiochip_add() failed\n", __func__);
    goto err;
  }

  for (n=0; n<ngpio; n++) {
    struct gpio_desc *desc;

    if ((priv->iodesc)[n].iotype == 2) { /* 2=output */
      desc = gpio_to_desc(priv->chip.base + n);
      gpiod_direction_output_raw(desc, (priv->iodesc)[n].initvalue);
    }

    pr_info("%s: %s, 0x%x, %d, %d, %d registered as gpio%d\n", __func__,
        (priv->iodesc)[n].name, (priv->iodesc)[n].portaddr, (priv->iodesc)[n].portbit,
        (priv->iodesc)[n].initvalue, (priv->iodesc)[n].iotype, priv->chip.base + n);
  }

err:
  return rc;
}

/**
 * nife200_gpio_remove - Remove method for the GPIO devices.
 */
static int nife200_gpio_remove(struct platform_device *pdev)
{
  struct priv_data *priv = platform_get_drvdata(pdev);

  /* Add per-device cleanup code here */

  gpiochip_remove(&priv->chip);
  kfree(priv);

  return 0;
}

/**
 * nife200_gpio_shutdown -
 */
static void nife200_gpio_shutdown(struct platform_device *pdev)
{
  /* Add code to shutdown the device here */
  pr_info("%s: Unsupported feature\n", __func__);
}

#ifdef CONFIG_PM
/**
 * nife200_gpio_suspend -
 */
static int nife200_gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
  /* Add code to suspend the device here */
  pr_info("%s: Unsupported feature\n", __func__);
  return 0;
}

/**
 * nife200_gpio_resume -
 */
static int nife200_gpio_resume(struct platform_device *pdev)
{
  /* Add code to resume the device here */
  pr_info("%s: Unsupported feature\n", __func__);
  return 0;
}
#else
/* No need to do suspend/resume if power management is disabled */
# define nife200_gpio_suspend  NULL
# define nife200_gpio_resume  NULL
#endif

/**
 *
 */
static struct platform_driver nife200_gpio_driver = {
  .probe    = nife200_gpio_probe,
  .remove    = nife200_gpio_remove,
  .shutdown  = nife200_gpio_shutdown,
  .suspend  = nife200_gpio_suspend,
  .resume    = nife200_gpio_resume,
  .driver  = {
    .name  = "gpio_nife200",
  },
};

/**
 * nife200_gpio_init -
 */
static int __init nife200_gpio_init(void)
{
  int rc = 0;

  rc = platform_driver_register(&nife200_gpio_driver);

  return rc;
}
module_init(nife200_gpio_init);

/**
 * nife200_gpio_exit -
 */
static void __exit nife200_gpio_exit(void)
{
  platform_driver_unregister(&nife200_gpio_driver);
}
module_exit(nife200_gpio_exit);

/* Information about this module */
MODULE_DESCRIPTION("NIFE200 gpio driver");
MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_LICENSE("GPL");

