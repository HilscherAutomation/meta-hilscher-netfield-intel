/*
* NCT7904D GPIO driver
*
* gpio-nct7904.c
*
* (C) Copyright 2015 Hilscher Gesellschaft fuer Systemautomation mbH
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


#define DRIVER_DESC "GPIO driver for NCT7904D"
#define DRIVER_NAME "gpio-nct7904"

#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/slab.h>

#include <linux/gpio/driver.h>

struct priv_data_gpio {
	uint8_t		*label;
	uint32_t	gpio_base;
	void		*iodesc;
};

struct priv_data {
	struct mutex		lock;
	struct device		*dev;
	struct i2c_client	*client;
	struct gpio_chip	chip;
	struct priv_data_gpio *priv_gpio;
};

#define VENDOR_ID_REG		0x7a /* Any bank */
#define NUVOTON_ID			0x50
#define CHIP_ID_REG			0x7b /* Any bank */
#define NCT7904_ID			0xc5
#define DEVICE_ID_REG		0x7c /* Any bank */
#define BANK_SEL_REG		0xff /* Any bank */

#define GPIO_BASE			(0xe8) /* bank 0 */
#define GPIO_EN_REG(ba)		(ba+0)
#define GPIO_DIR_REG(ba)	(ba+1)
#define GPIO_OUT_REG(ba)	(ba+2)
#define GPIO_IN_REG(ba)		(ba+3)

#define get_bit(client, reg, bit) ({ \
	(i2c_smbus_read_byte_data(client, reg) & (1<<bit)) ? 1 : 0; \
})
#define set_bit(client, reg, bit) ({ \
	int regval = i2c_smbus_read_byte_data(client, reg); \
	i2c_smbus_write_byte_data(client, reg, regval | (1<<bit)); \
})
#define clear_bit(client, reg, bit) ({ \
	int regval = i2c_smbus_read_byte_data(client, reg); \
	i2c_smbus_write_byte_data(client, reg, regval & ~(1<<bit)); \
})

#define modify_access_data(ba, offset) ({ \
	if (offset >= 7) { \
		ba += 4; \
		offset -= 7; \
	} \
})

static int nct7904_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct priv_data *pdata = gpiochip_get_data(chip);
	unsigned char ba = GPIO_BASE;
	int rc = 0;

	modify_access_data(ba, offset);

	mutex_lock(&pdata->lock);
	if (get_bit(pdata->client, GPIO_EN_REG(ba), offset) == 0) {
		dev_err(pdata->dev, "GPIO%d is disabled\n", chip->base+offset);
		rc = -ENODEV;
	}
	mutex_unlock(&pdata->lock);

	return (rc) ? rc : 0;
}

static int nct7904_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct priv_data *pdata = gpiochip_get_data(chip);
	unsigned char ba = GPIO_BASE;
	int val;

	modify_access_data(ba, offset);

	mutex_lock(&pdata->lock);
	val = get_bit(pdata->client, GPIO_IN_REG(ba), offset);
	mutex_unlock(&pdata->lock);

	return val;
}

static void nct7904_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	struct priv_data *pdata = gpiochip_get_data(chip);
	unsigned char ba = GPIO_BASE;

	modify_access_data(ba, offset);

	mutex_lock(&pdata->lock);
	if (val)
		set_bit(pdata->client, GPIO_OUT_REG(ba), offset);
	else
		clear_bit(pdata->client, GPIO_OUT_REG(ba), offset);
	mutex_unlock(&pdata->lock);
}

static int nct7904_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct priv_data *pdata = gpiochip_get_data(chip);
	unsigned char ba = GPIO_BASE;
	int val;

	modify_access_data(ba, offset);

	mutex_lock(&pdata->lock);
	val = get_bit(pdata->client, GPIO_DIR_REG(ba), offset);
	mutex_unlock(&pdata->lock);


	return val;
}

static int nct7904_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct priv_data *pdata = gpiochip_get_data(chip);
	unsigned char ba = GPIO_BASE;

	modify_access_data(ba, offset);

	mutex_lock(&pdata->lock);
	set_bit(pdata->client, GPIO_DIR_REG(ba), offset);
	mutex_unlock(&pdata->lock);

	return 0;
}

static int nct7904_gpio_direction_output(struct gpio_chip *chip, unsigned offset, int val)
{
	struct priv_data *pdata = gpiochip_get_data(chip);
	unsigned char ba = GPIO_BASE;

	modify_access_data(ba, offset);

	nct7904_gpio_set(chip, offset, val);

	mutex_lock(&pdata->lock);
	clear_bit(pdata->client, GPIO_DIR_REG(ba), offset);
	mutex_unlock(&pdata->lock);

	return 0;
}

static int nct7904_gpio_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct priv_data *pdata = NULL;
	int rc;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -ENODEV;

	if (i2c_smbus_read_byte_data(client, VENDOR_ID_REG) != NUVOTON_ID ||
		i2c_smbus_read_byte_data(client, CHIP_ID_REG) != NCT7904_ID ||
		(i2c_smbus_read_byte_data(client, DEVICE_ID_REG) & 0xf0) != 0x50)
		return -ENODEV;

	i2c_smbus_write_byte_data(client, BANK_SEL_REG, 0x0); /* select bank 0 */

	pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	pdata->priv_gpio = dev_get_platdata(&client->dev);

	pdata->dev = &client->dev;
	pdata->client = client;

	pdata->chip.base = -1; /* dynamic ID allocation */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,5,0)
	pdata->chip.dev = pdata->dev;
#endif
	pdata->chip.ngpio = id->driver_data;
	pdata->chip.can_sleep = true;
	pdata->chip.owner = THIS_MODULE;

	pdata->chip.request = nct7904_gpio_request;
	pdata->chip.get = nct7904_gpio_get;
	pdata->chip.set = nct7904_gpio_set;
	pdata->chip.get_direction = nct7904_gpio_get_direction;
	pdata->chip.direction_input = nct7904_gpio_direction_input;
	pdata->chip.direction_output = nct7904_gpio_direction_output;


	/* Overwrite chip data with private data */
	if (pdata->priv_gpio != NULL) {
		pdata->chip.label = pdata->priv_gpio->label;
		pdata->chip.base = pdata->priv_gpio->gpio_base;
	}

	dev_set_drvdata(pdata->dev, pdata);

	mutex_init(&pdata->lock);

	rc = gpiochip_add_data(&pdata->chip, pdata);
	if (rc)
		goto err_out;

	dev_info(pdata->dev, "%s found at 0x%02x", pdata->chip.label, client->addr);

	if (pdata->priv_gpio != NULL)
		pdata->priv_gpio->gpio_base = pdata->chip.base;

	return 0;

err_out:
	return rc;
}

static int nct7904_gpio_remove(struct i2c_client *client)
{
	struct priv_data *pdata = dev_get_drvdata(&client->dev);

	gpiochip_remove(&pdata->chip);
	mutex_destroy(&pdata->lock);

	return 0;
}

static const struct i2c_device_id nct7904_gpio_id[] = {
	{ "nct7904", 11 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, nct7904_gpio_id);

static struct i2c_driver nct7904_gpio_driver = {
	.probe = nct7904_gpio_probe,
	.remove = nct7904_gpio_remove,
	.id_table = nct7904_gpio_id,
	.driver = {
		.name = DRIVER_NAME,
	},
};

static int __init nct7904_gpio_init(void)
{
	pr_info("%s: %s\n", DRIVER_NAME, DRIVER_DESC);
	return i2c_add_driver(&nct7904_gpio_driver);
}
module_init(nct7904_gpio_init);

static void __exit nct7904_gpio_exit(void)
{
	i2c_del_driver(&nct7904_gpio_driver);
}
module_exit(nct7904_gpio_exit);

MODULE_AUTHOR("Hilscher Gesellschaft fuer Systemautomation mbH");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

