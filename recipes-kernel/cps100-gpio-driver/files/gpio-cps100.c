/*
* GPIO driver for CPS100 platforms.
*
* gpio-cps100.c
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


#define DRIVER_DESC "GPIO driver for the CPS100 platform"
#define DRIVER_NAME "gpio-cps100"

#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/ioport.h>


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

static struct priv_data {
	char label[32];
	spinlock_t lock;
	struct gpio_chip chip;
	struct priv_data_gpio *priv_gpio;
	void __iomem *viobase;
};

/**
 * cps100_gpio_get - Read the value of the specified GPIO.
 */
static int cps100_gpio_get(struct gpio_chip *chip, unsigned off)
{
	struct priv_data *priv = container_of(chip, struct priv_data, chip);
	struct io_desc *iodesc = priv->priv_gpio->iodesc;
	int regval;

	spin_lock(&priv->lock);
	regval = ioread32((priv->viobase)+(iodesc[off].portaddr)+8);
	spin_unlock(&priv->lock);

	return (regval & 0x1) ? 1 : 0;
}

/**
 * cps100_gpio_set - Set a new value of the specified GPIO.
 */
static void cps100_gpio_set(struct gpio_chip *chip, unsigned off, int val)
{
	struct priv_data *priv = container_of(chip, struct priv_data, chip);
	struct io_desc *iodesc = priv->priv_gpio->iodesc;
	int regval;

	spin_lock(&priv->lock);
	regval = ioread32((priv->viobase)+(iodesc[off].portaddr)+8);
	(val) ? (regval |= 0x1) : (regval &= ~0x1);
	iowrite32(regval, (priv->viobase)+(iodesc[off].portaddr)+8);
	spin_unlock(&priv->lock);
}

/**
 * cps100_gpio_get_dir - Get the direction of the specified GPIO.
 */
static int cps100_gpio_get_dir(struct gpio_chip *chip, unsigned off)
{
	struct priv_data *priv = container_of(chip, struct priv_data, chip);
	struct io_desc *iodesc = priv->priv_gpio->iodesc;
	int regval;

	spin_lock(&priv->lock);
	regval = ioread32((priv->viobase)+(iodesc[off].portaddr)+8);
	spin_unlock(&priv->lock);

	return (regval & 0x2) ? 0 : 1;
}

/**
 * cps100_gpio_dir_in - Set the direction of the specified GPIO as input.
 */
static int cps100_gpio_dir_in(struct gpio_chip *chip, unsigned off)
{
	struct priv_data *priv = container_of(chip, struct priv_data, chip);
	struct io_desc *iodesc = priv->priv_gpio->iodesc;

	if (iodesc[off].iotype == 2) { /* 2=output */
		pr_err("Error: GPIO%d is a dedicated output!\n", chip->base+off);
		return -EIO;
	}

	spin_lock(&priv->lock);
	iowrite32(0x2, (priv->viobase)+(iodesc[off].portaddr)+8);
	spin_unlock(&priv->lock);

	return 0;
}

/**
 * cps100_gpio_dir_out - Set the direction of the specified GPIO as output.
 */
static int cps100_gpio_dir_out(struct gpio_chip *chip, unsigned off, int val)
{
	struct priv_data *priv = container_of(chip, struct priv_data, chip);
	struct io_desc *iodesc = priv->priv_gpio->iodesc;

	if (iodesc[off].iotype == 1) { /* 1=input */
		pr_err("Error: GPIO%d is a dedicated input!\n", chip->base+off);
		return -EIO;
	}

	spin_lock(&priv->lock);
	iowrite32((val) ? 1 : 0, (priv->viobase)+(iodesc[off].portaddr)+8);
	spin_unlock(&priv->lock);

	return 0;
}

/**
 * cps100_gpio_probe - Probe method for the GPIO devices.
 */
static int cps100_gpio_probe(struct platform_device *pdev)
{
	struct priv_data *priv = NULL;
	struct io_desc *iodesc;
	int rc = 0, n, ngpio;

	/* Add per-device initialization code here */

	/* Allocate zero initialized memory for private data */
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		pr_err("%s: devm_kzalloc() failed\n", __func__);
		return -ENOMEM;
	}

	/* Link the gpio descriptions for private use */
	priv->priv_gpio = dev_get_platdata(&pdev->dev);

	/* Count the number of gpios */
	iodesc = priv->priv_gpio->iodesc;
	while (*(char*)iodesc != '\0') iodesc++;
	ngpio = (iodesc - (struct io_desc *)priv->priv_gpio->iodesc);

	/* Configure the chip */
	priv->chip.base  = -1; /* dynamic base address */
	priv->chip.ngpio = ngpio;
	priv->chip.can_sleep = false;
	priv->chip.owner = THIS_MODULE;

	priv->chip.get = cps100_gpio_get;
	priv->chip.set = cps100_gpio_set;
	priv->chip.get_direction = cps100_gpio_get_dir;
	priv->chip.direction_input = cps100_gpio_dir_in;
	priv->chip.direction_output = cps100_gpio_dir_out;

	/* Overwrite chip data with private data */
	if (priv->priv_gpio != NULL) {
		priv->chip.label = priv->priv_gpio->label;
		priv->chip.base = priv->priv_gpio->gpio_base;
	}

	spin_lock_init(&priv->lock);

	platform_set_drvdata(pdev, priv);

	/* Initialize the gpio controller */
	{
		uint32_t gbase, iobase, regval;
		/* Requesting the base address GBASE (S5) */
		outl((1<<31) | (0<<24) | (0<<16) | (31<<11) | (0<<8) | 0x48, 0xcf8);
		gbase = inl(0xcfc) & 0xff00;
		gbase += 0x80; /* GBASE of S5-GPIOs */

		/* Requesting a virtual base address of IOBASE (S5) */
		outl((1<<31) | (0<<24) | (0<<16) | (31<<11) | (0<<8) | 0x4c, 0xcf8);
		iobase = inl(0xcfc) & 0xffffc000;
		iobase += 0x2000; /* IOBASE of S5-GPIOs */

		/* Request and map the IOBASE */
		if(request_mem_region(iobase, 0x1000, DRIVER_NAME) == NULL) {
			pr_info("%s: request_mem_region() failed\n", __func__);
		    return -ENODEV;
		}
		priv->viobase = ioremap(iobase, 0x1000);

		pr_info("%s: gbase-s5=0x%08x, iobase-s5=0x%08x, viobase-s5=0x%08x\n", __func__, gbase, iobase, priv->viobase);

		/* Enable the memory map mode for the GPIOs (GPIO_DFX[0..7]) */
		pr_info("%s: Enabling memory mapped mode for GPI0..3 and GPO0..3\n", __func__);
		regval = inl(gbase+0x0);
		outl(regval & ~0x3fc00000, gbase+0x0);
	}

	/* Preconfigure the GPIOs */
	iodesc = priv->priv_gpio->iodesc;
	for (n=0; n<ngpio; n++) {
		if (iodesc[n].iotype == 1) { /* 1=input */
			cps100_gpio_dir_in(&priv->chip, n);
		}
		else if (iodesc[n].iotype == 2) { /* 2=output */
			cps100_gpio_dir_out(&priv->chip, n, iodesc[n].initvalue);
		}
	}

	rc = gpiochip_add(&priv->chip);
	if (rc) {
		pr_err("%s: gpiochip_add() failed\n", __func__);
		goto err_out;
	}

	if (priv->priv_gpio != NULL)
		priv->priv_gpio->gpio_base = priv->chip.base;

	return 0;

err_out:
	return rc;
}

/**
 * cps100_gpio_remove - Remove method for the GPIO devices.
 */
static int cps100_gpio_remove(struct platform_device *pdev)
{
	struct priv_data *priv = platform_get_drvdata(pdev);

	/* Add per-device cleanup code here */

	iounmap(priv->viobase);
	gpiochip_remove(&priv->chip);

	return 0;
}

/**
 * cps100_gpio_shutdown -
 */
static void cps100_gpio_shutdown(struct platform_device *pdev)
{
	/* Add code to shutdown the device here */
	pr_info("%s: Unsupported feature\n", __func__);
}

#ifdef CONFIG_PM
/**
 * cps100_gpio_suspend -
 */
static int cps100_gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* Add code to suspend the device here */
	pr_info("%s: Unsupported feature\n", __func__);
	return 0;
}

/**
 * cps100_gpio_resume -
 */
static int cps100_gpio_resume(struct platform_device *pdev)
{
	/* Add code to resume the device here */
	pr_info("%s: Unsupported feature\n", __func__);
	return 0;
}
#else
/* No need to do suspend/resume if power management is disabled */
# define cps100_gpio_suspend  NULL
# define cps100_gpio_resume  NULL
#endif

/**
 *
 */
static struct platform_driver cps100_gpio_driver = {
	.probe		= cps100_gpio_probe,
	.remove		= cps100_gpio_remove,
	.shutdown	= cps100_gpio_shutdown,
	.suspend	= cps100_gpio_suspend,
	.resume		= cps100_gpio_resume,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};

/**
 * cps100_gpio_init -
 */
static int __init cps100_gpio_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	return platform_driver_register(&cps100_gpio_driver);
}
module_init(cps100_gpio_init);

/**
 * cps100_gpio_exit -
 */
static void __exit cps100_gpio_exit(void)
{
	platform_driver_unregister(&cps100_gpio_driver);
}
module_exit(cps100_gpio_exit);

/* Information about this module */
MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

