/*
 * GPIO driver which supports following Nuvoton chips:
 *	NCT5104D
 *	NCT6106D
 *	NCT6796D
 *
 * Author: 
 *		   Tasanakorn Phaipool <tasanakorn@gmail.com>
 *
 *	  	   Sheng-Yuan Huang <syhuang3@nuvoton.com>   (Modfiy for nct6106d in 2017)
 *                                                   (Modfiy for nct6796d in 2019)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/version.h>

#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irqnr.h>
#include <linux/irq.h>
#include <linux/irq_work.h>
#include <linux/irqdomain.h>

#define DRVNAME "gpio-nct6106d"

/*
 * Super-I/O registers
 */
#define SIO_LDSEL			0x07	/* Logical device select */
#define SIO_CHIPID			0x20	/* Chaip ID (2 bytes) */
#define SIO_GPIO_ENABLE		0x30	/* GPIO enable */
#define SIO_GPIO0_MODE		0xE0	/* GPIO0 Mode OpenDrain/Push-Pull */
#define SIO_GPIO1_MODE		0xE1	/* GPIO1 Mode OpenDrain/Push-Pull */

//#define SIO_LD_GPIO			0x07	/* GPIO logical device */
#define SIO_LD_GPIO_MODE	0x0F	/* GPIO mode control device */
#define SIO_UNLOCK_KEY		0x87	/* Key to enable Super-I/O */
#define SIO_LOCK_KEY			0xAA	/* Key to disable Super-I/O */

#define SIO_ID_MASK			0xFFF0
#define SIO_NCT5104D_ID		(0x1061&SIO_ID_MASK)	/* Chip ID */
#define SIO_NCT5124D_ID         (0xD2C3&SIO_ID_MASK)    /* Chip ID */
#define SIO_NCT6106D_ID		(0xc452&SIO_ID_MASK)	/* Chip ID */
#define SIO_NCT6126D_ID		(0xd280&SIO_ID_MASK)	/* Chip ID */
#define SIO_NCT6796D_E_ID	(0xD42A&SIO_ID_MASK)	/* Chip ID */
#define SIO_NCT6791D_ID	    (0xC800&SIO_ID_MASK)	/* Chip ID */

#define ADVANTECH_NCT_GPIO_VER              "1.1.1"

/* Macro DECLARE_TASKLET_OLD exists for compatibiity.
 * See https://lwn.net/Articles/830964/
 */
#ifndef DECLARE_TASKLET_OLD
#define DECLARE_TASKLET_OLD(arg1, arg2) DECLARE_TASKLET(arg1, arg2, 0L)
#endif

struct _sw_irq_provider
{
	u8 pre_value;
	int irq_base;
	int irq_num;
	uint32_t irq_mask_reg;
	uint32_t irq_rising_reg;
	uint32_t irq_falling_reg;
	struct irq_chip irq_chip;
	struct irq_domain	*irq_domain;
	struct platform_device *platform_dev;
	spinlock_t lock;
};

/* bool for sw irq*/
static bool sw_irq_enable = false;
module_param(sw_irq_enable, bool, S_IRUGO);
MODULE_PARM_DESC(sw_irq_enable, "enable sw irq interrupt");

/* Timer to detect changes */
static struct timer_list *timer = NULL;
static unsigned int polling_rate = 500;	//unit: ms, default 500ms
static bool timer_stop = true;
module_param(polling_rate, uint, S_IRUGO);
MODULE_PARM_DESC(polling_rate, "polling rate for detect changes");

/* debug use */
static bool debug = false;
module_param(debug, bool, S_IRUGO);
MODULE_PARM_DESC(debug, "debug mode enable");
#define debug_print(fmt, ...) \
            do { if (debug) printk(fmt, __VA_ARGS__); else pr_debug(fmt, __VA_ARGS__); } while (0)

enum chips { nct5104d, nct5124d, nct6106d, nct6796d_e, nct6791d}; 

static const char * const nct5104d_names[] = {
	"nct5104d",
	"nct5124d",   
	"nct6106d",
	"nct6796d_e",  
	"nct6791d"      
};

struct _nct5104d_sio {
	int addr;
	enum chips type;
};

struct _nct5104d_gpio_bank {
	struct gpio_chip chip;
    unsigned char logic_dev;
	unsigned int regbase;
	struct _sw_irq_provider sw_irq_provider;
	struct _nct5104d_gpio_data *data;
};

struct _nct5104d_gpio_data {
	struct _nct5104d_sio *sio;
	int nr_bank;
	struct _nct5104d_gpio_bank *bank;
};

static struct _nct5104d_gpio_data* g_nct5104d_gpio_data = NULL;
/*
 * Super-I/O functions.
 */

static inline int superio_inb(int base, int reg)
{
	outb(reg, base);
	return inb(base + 1);
}

static int superio_inw(int base, int reg)
{
	int val;

	outb(reg++, base);
	val = inb(base + 1) << 8;
	outb(reg, base);
	val |= inb(base + 1);

	return val;
}

static inline void superio_outb(int base, int reg, int val)
{
	outb(reg, base);
	outb(val, base + 1);
}

static inline int superio_enter(int base)
{
	/* Don't step on other drivers' I/O space by accident. */
	if (!request_muxed_region(base, 2, DRVNAME)) {
		pr_err(DRVNAME "I/O address 0x%04x already in use\n", base);
		return -EBUSY;
	}

	/* According to the datasheet the key must be send twice. */
	outb(SIO_UNLOCK_KEY, base);
	outb(SIO_UNLOCK_KEY, base);

	return 0;
}

static inline void superio_select(int base, int ld)  //???
{
	outb(SIO_LDSEL, base);
	outb(ld, base + 1);
}

static inline void superio_exit(int base)
{
	outb(SIO_LOCK_KEY, base);
	release_region(base, 2);
}

/*
 * GPIO chip.
 */

static int nct5104d_gpio_direction_in(struct gpio_chip *chip, unsigned offset);
static int nct5104d_gpio_get(struct gpio_chip *chip, unsigned offset);
static int nct5104d_gpio_direction_out(struct gpio_chip *chip,
				     unsigned offset, int value);
static void nct5104d_gpio_set(struct gpio_chip *chip, unsigned offset, int value);

#define NCT5104D_GPIO_BANK(_base, _ngpio, _ld, _regbase, _label)			\
	{								\
		.chip = {						\
			.label            = _label,			\
			.owner            = THIS_MODULE,		\
			.direction_input  = nct5104d_gpio_direction_in,	\
			.get              = nct5104d_gpio_get,		\
			.direction_output = nct5104d_gpio_direction_out,	\
			.set              = nct5104d_gpio_set,		\
			.base             = _base,			\
			.ngpio            = _ngpio,			\
			.can_sleep        = false,			\
		},							\
		.logic_dev = _ld,                  \
		.regbase = _regbase,					\
	}

#define GPIO_DIR(base) (base + 0)
#define GPIO_DATA(base) (base + 1)

/* Only bank 2,4,6 use in advantech devices */
static struct _nct5104d_gpio_bank nct5104d_gpio_bank[] = {
    //NCT5104D_GPIO_BANK(0 , 8, 7, 0xE0, "GPIO0"),        //GPIO0
    //NCT5104D_GPIO_BANK(10, 8, 7, 0xE4, "GPIO1"),        //GPIO1
    NCT5104D_GPIO_BANK(20, 8, 7, 0xE8, "GPIO2"),          //GPIO2
    NCT5104D_GPIO_BANK(30, 8, 7, 0xEC, "GPIO3"),          //GPIO3
    NCT5104D_GPIO_BANK(40, 8, 7, 0xF0, "GPIO4"),          //GPIO4
    //NCT5104D_GPIO_BANK(50, 8, 7, 0xF4, "GPIO5"),	  //Skip GPIO5
    NCT5104D_GPIO_BANK(60, 8, 7, 0xF8, "GPIO6"),	  //Skip GPIO6
    //NCT5104D_GPIO_BANK(70, 8, 7, 0xFC, "GPIO7"),	  //No GPIO7
    NCT5104D_GPIO_BANK(80, 8, 9, 0xF0, "GPIO8"),          //GPIO8
	
};

static struct _nct5104d_gpio_bank nct5124d_gpio_bank[] = {
    NCT5104D_GPIO_BANK(0 , 8, 7, 0xE0, "GPIO0"),          //GPIO0
    //NCT5104D_GPIO_BANK(10, 8, 7, 0xE4, "GPIO1"),        //skip GPIO1
    //NCT5104D_GPIO_BANK(60, 8, 7, 0xF8, "GPIO6"),        //skip GPIO6
    //NCT5104D_GPIO_BANK(70, 8, 9, 0xF4, "GPIO9"),        //skip GPIO9
    //NCT5104D_GPIO_BANK(80, 8, 9, 0xF8, "GPIOA"),        //skip GPIOA
    //NCT5104D_GPIO_BANK(80, 8, 9, 0xFC, "GPIOB"),        //skip GPIOB

};


static struct _nct5104d_gpio_bank nct6796d_e_gpio_bank[] = {
    //NCT5104D_GPIO_BANK(20, 8, 9, 0xE0, "GPIO2"),        //GPIO2
    NCT5104D_GPIO_BANK(30, 7, 9, 0xE4, "GPIO3"),          //GPIO3
    NCT5104D_GPIO_BANK(40, 8, 9, 0xF0, "GPIO4"),          //GPIO4
    NCT5104D_GPIO_BANK(50, 8, 9, 0xF4, "GPIO5"),          //GPIO5
    NCT5104D_GPIO_BANK(60, 8, 7, 0xF4, "GPIO6"),          //GPIO6
    NCT5104D_GPIO_BANK(70, 8, 7, 0xE0, "GPIO7"),          //GPIO7
    NCT5104D_GPIO_BANK(80, 8, 7, 0xE4, "GPIO8"),          //GPIO8
    NCT5104D_GPIO_BANK(90, 4, 7, 0xE8, "GPIO9"),          //GPIO9
};


static struct _nct5104d_gpio_bank nct6791d_gpio_bank[] = {
    NCT5104D_GPIO_BANK(20, 8, 9, 0xE0, "GPIO2"),          //GPIO2
    NCT5104D_GPIO_BANK(30, 7, 9, 0xE4, "GPIO3"),          //GPIO3
    NCT5104D_GPIO_BANK(40, 8, 9, 0xF0, "GPIO4"),          //GPIO4
    NCT5104D_GPIO_BANK(50, 8, 9, 0xF4, "GPIO5"),          //GPIO5
    NCT5104D_GPIO_BANK(60, 8, 7, 0xF4, "GPIO6"),          //GPIO6
    NCT5104D_GPIO_BANK(70, 7, 7, 0xE0, "GPIO7"),          //GPIO7
    NCT5104D_GPIO_BANK(80, 8, 7, 0xE4, "GPIO8"),          //GPIO8
};



static int nct5104d_gpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
	int err;
	struct _nct5104d_gpio_bank *bank =
		container_of(chip, struct _nct5104d_gpio_bank, chip);
	struct _nct5104d_sio *sio = bank->data->sio;
	u8 dir;

	err = superio_enter(sio->addr);
	if (err)
		return err;
	superio_select(sio->addr, bank->logic_dev);

	dir = superio_inb(sio->addr, GPIO_DIR(bank->regbase));
	dir |= (1 << offset);
	superio_outb(sio->addr, GPIO_DIR(bank->regbase), dir);

	superio_exit(sio->addr);

	return 0;
}

static int nct5104d_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	int err;
	struct _nct5104d_gpio_bank *bank =
		container_of(chip, struct _nct5104d_gpio_bank, chip);
	struct _nct5104d_sio *sio = bank->data->sio;
	u8 data;

	err = superio_enter(sio->addr);
	if (err)
		return err;
	superio_select(sio->addr, bank->logic_dev);

	data = superio_inb(sio->addr, GPIO_DATA(bank->regbase));

	superio_exit(sio->addr);

	return !!(data & 1 << offset);
}

static int nct5104d_gpio_direction_out(struct gpio_chip *chip,
				     unsigned offset, int value)
{
	int err;
	struct _nct5104d_gpio_bank *bank =
		container_of(chip, struct _nct5104d_gpio_bank, chip);
	struct _nct5104d_sio *sio = bank->data->sio;
	u8 dir, data_out;

	err = superio_enter(sio->addr);
	if (err)
		return err;
	superio_select(sio->addr, bank->logic_dev);

	data_out = superio_inb(sio->addr, GPIO_DATA(bank->regbase));
	if (value)
		data_out |= (1 << offset);
	else
		data_out &= ~(1 << offset);
	superio_outb(sio->addr, GPIO_DATA(bank->regbase), data_out);

	dir = superio_inb(sio->addr, GPIO_DIR(bank->regbase));
	dir &= ~(1 << offset);
	superio_outb(sio->addr, GPIO_DIR(bank->regbase), dir);

	superio_exit(sio->addr);

	return 0;
}

static void nct5104d_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	int err;
	struct _nct5104d_gpio_bank *bank =
		container_of(chip, struct _nct5104d_gpio_bank, chip);
	struct _nct5104d_sio *sio = bank->data->sio;
	u8 data_out;

	err = superio_enter(sio->addr);
	if (err)
		return;
	superio_select(sio->addr, bank->logic_dev);

	data_out = superio_inb(sio->addr, GPIO_DATA(bank->regbase));
	if (value)
		data_out |= (1 << offset);
	else
		data_out &= ~(1 << offset);
	superio_outb(sio->addr, GPIO_DATA(bank->regbase), data_out);

	superio_exit(sio->addr);
}

/* we check the different of values to generate irq for edge event */
static void process_data(bool is_frist)
{
	int i = 0, j = 0, err = 0;

    debug_print("nct5104d: [CPU#%d] produce data\n", smp_processor_id());
	for (i = 0; i < g_nct5104d_gpio_data->nr_bank; i++) {
		struct _nct5104d_gpio_bank *bank = g_nct5104d_gpio_data->bank + i;
		struct _nct5104d_sio *sio = bank->data->sio;
		u8 data;
		err = superio_enter(sio->addr);
		if (err){
			pr_err("superio_enter in bank=%d error", i);
			continue;
		}
		superio_select(sio->addr, bank->logic_dev);

		data = superio_inb(sio->addr, GPIO_DATA(bank->regbase));	//current level
		if(is_frist) {
			debug_print("bank %d = 0x%02x \n", i, data);
		}
		else {
			if (bank->sw_irq_provider.pre_value != data) {
				u8 changed = bank->sw_irq_provider.pre_value ^ data;
				u8 dir = 0;
				dir = superio_inb(sio->addr, GPIO_DIR(bank->regbase));	//current directions
				changed = changed & dir;	// only input need irq event
				changed = changed & (bank->sw_irq_provider.irq_rising_reg | bank->sw_irq_provider.irq_falling_reg); //only edge registered (rising+falling)

				debug_print("bank %d from 0x%02x to 0x%02x \n", i, bank->sw_irq_provider.pre_value, data);
				for (j = 0; j < bank->sw_irq_provider.irq_num; j++)
				{
					if (changed & (1 << j)) {
						u8 tmp = data & (1 << j);
						bool need_irq = false;
						debug_print("%s%d level changed.\n", bank->chip.label, j);
						
						if (tmp) {
							if (tmp & bank->sw_irq_provider.irq_rising_reg) need_irq = true;
						} else {
							if (bank->sw_irq_provider.irq_falling_reg & (1 << j)) need_irq = true;
						}
						if (need_irq)
						{
							err = generic_handle_irq(bank->sw_irq_provider.irq_base + j);
							if(err != 0) pr_err("%s:%d err=%d\n", __FUNCTION__, __LINE__, err);
						}
					}
				}
			}
		}
		bank->sw_irq_provider.pre_value = data;

		superio_exit(sio->addr);
	}

    debug_print("nct5104d: [CPU#%d] scheduling tasklet\n", smp_processor_id());
}

static void timer_handler(struct timer_list *__timer)
{
    ktime_t tv_start, tv_end;
    s64 nsecs;

    debug_print("nct5104d: [CPU#%d] enter %s\n", smp_processor_id(), __func__);
    /* We are using a kernel timer to simulate a hard-irq, so we must expect
     * to be in softirq context here.
     */
    WARN_ON_ONCE(!in_softirq());

    /* Disable interrupts for this CPU to simulate real interrupt context */
    local_irq_disable();

    tv_start = ktime_get();
    process_data(false);
    tv_end = ktime_get();

    nsecs = (s64) ktime_to_ns(ktime_sub(tv_end, tv_start));

    debug_print("nct5104d: [CPU#%d] %s in_irq: %llu usec\n", smp_processor_id(),
            __func__, (unsigned long long) nsecs >> 10);

	if (!timer_stop) mod_timer(timer, jiffies + msecs_to_jiffies(polling_rate));

    local_irq_enable();
}

/* we always check timer to stop when all gpio pin mask */
static void nct5104d_check_timer(struct _nct5104d_gpio_data* nct5104d_gpio_data)
{
	int i;
	for (i = 0; i < nct5104d_gpio_data->nr_bank; i++) {
		struct _nct5104d_gpio_bank *bank = nct5104d_gpio_data->bank + i;
		if (bank->sw_irq_provider.irq_mask_reg != UINT_MAX){
			if(timer_stop) {
				timer_stop = false;
				process_data(true);
				mod_timer(timer, jiffies + msecs_to_jiffies(polling_rate));
			}
			return;
		}
	}
	timer_stop = true;
	return;
}

/* when enable monitor gpio pin, need to unmask irq_mask_reg bit */
void nct5104d_irq_unmask(struct irq_data *data)
{
	struct _sw_irq_provider *devp = data->chip_data;
	struct _nct5104d_gpio_data *nct5104d_gpio_data = platform_get_drvdata(devp->platform_dev);
	uint8_t irq_index = data->hwirq;
	debug_print("%s:%d irq_num=%d hw_irq=%ld\n", __FUNCTION__, __LINE__, data->irq, data->hwirq);
	spin_lock(&devp->lock);
	devp->irq_mask_reg &= ~(1<<irq_index);
	spin_unlock(&devp->lock);
	nct5104d_check_timer(nct5104d_gpio_data);
}

/* when disable monitor gpio pin, need to mask irq_mask_reg bit */
void nct5104d_irq_mask(struct irq_data *data)
{
	struct _sw_irq_provider *devp = data->chip_data;
	struct _nct5104d_gpio_data *nct5104d_gpio_data = platform_get_drvdata(devp->platform_dev);
	uint8_t irq_index = data->hwirq;
	debug_print("%s:%d irq_num=%d hw_irq=%ld\n", __FUNCTION__, __LINE__, data->irq, data->hwirq);
	spin_lock(&devp->lock);
	devp->irq_mask_reg |= (1<<irq_index);
	spin_unlock(&devp->lock);
	nct5104d_check_timer(nct5104d_gpio_data);
}

int nct5104d_irq_set_affinity(struct irq_data *data, const struct cpumask *dest, bool force)
{
	debug_print("%s:%d \n", __FUNCTION__, __LINE__);
	/*set affinity*/
	return 0;
}

/* when enable monitor gpio pin, need to set irq_rising_reg/irq_falling_reg bits for process_data use */
int nct5104d_irq_set_type(struct irq_data *data, unsigned int flow_type)
{
	struct _sw_irq_provider *devp = data->chip_data;
	uint8_t bit_shift = data->hwirq;
	uint32_t rising = 0;
	uint32_t falling = 0;
	int ret = 0;

	debug_print("%s:%d irq_num=%d\n", __FUNCTION__, __LINE__, bit_shift);
	spin_lock(&devp->lock);
	rising = devp->irq_rising_reg;
	falling = devp->irq_falling_reg;
	if (flow_type == IRQ_TYPE_EDGE_RISING) {
		rising |= (1<<bit_shift);
		falling &= ~(1<<bit_shift);
	} else if (flow_type == IRQ_TYPE_EDGE_FALLING) {
		falling |= (1<<bit_shift);
		rising &= ~(1<<bit_shift);
	} else if (flow_type == IRQ_TYPE_EDGE_BOTH) {
		falling |= (1<<bit_shift);
		rising |= (1<<bit_shift);
	} else if(flow_type == IRQ_TYPE_NONE) {
		falling &= ~(1<<bit_shift);
		rising &= ~(1<<bit_shift);
	}
	else
	{
		ret = -1;
		goto unlock;
	}

	devp->irq_rising_reg = rising;
	devp->irq_falling_reg = falling;
unlock:
	spin_unlock(&devp->lock);

	return ret;
}

static int gpio_nct5104d_to_irq(struct gpio_chip *gc, unsigned int offset)
{
	struct _nct5104d_gpio_bank *bank = gpiochip_get_data(gc);
	int irq = bank->sw_irq_provider.irq_base + offset;
	debug_print("%s:%d \n", __FUNCTION__, __LINE__);
	return irq;
}

static int nct5104d_irq_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	struct _sw_irq_provider *devp = (struct _sw_irq_provider *)(d->host_data);
	irq_set_chip_data(irq, (void *)devp);
	irq_set_chip_and_handler_name(irq, &devp->irq_chip, handle_level_irq, devp->platform_dev->name);
	irq_set_irq_type(irq, IRQ_TYPE_EDGE_BOTH);
	debug_print("%s:%d irq=%d \n", __FUNCTION__, __LINE__, irq);
	return 0;
}

static const struct irq_domain_ops nct5104d_irq_ops = {
	.map = nct5104d_irq_map,
	.xlate = irq_domain_xlate_onetwocell,
};

/*
 * Platform device and driver.
 */

static int nct5104d_gpio_probe(struct platform_device *pdev)
{
	int err;
	int i;
	struct _nct5104d_sio *sio = pdev->dev.platform_data;
	struct _nct5104d_gpio_data *data;
	u8 gpio_cfg;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	err = superio_enter(sio->addr);

	switch (sio->type) {
	case nct6106d:
	case nct5104d:
		/* GP65 must to set push-pull to control LED in UNO-148 / UNO-348
		   Logical Device F, CR E6 bit [5] = "0"  */
		superio_select(sio->addr, 0xf);
		gpio_cfg = superio_inb(sio->addr, 0xE6);
		gpio_cfg &= 0xDF;
		superio_outb(sio->addr, 0xE6, gpio_cfg);

		/* GP65 set function to GPIO CR 1C bit [6] = "1" */
                /* select to global register*/
                superio_select(sio->addr, 0x0);
                gpio_cfg = superio_inb(sio->addr, 0x1C);
                gpio_cfg |= 0x40;
                superio_outb(sio->addr, 0x1C, gpio_cfg);

		data->nr_bank = ARRAY_SIZE(nct5104d_gpio_bank);
		data->bank = nct5104d_gpio_bank;

   		/* enable GPIO.It seems no side-effect if we enable all of them */
		superio_select(sio->addr, 7);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0xEF;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);
		break;

    case nct5124d:
		data->nr_bank = ARRAY_SIZE(nct5124d_gpio_bank);
                data->bank = nct5124d_gpio_bank;
		/* enable GPIO.It seems no side-effect if we enable all of them */
		superio_select(sio->addr, 7);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0x0F;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);

		superio_select(sio->addr, 9);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0x0F;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);
		break;

    case nct6796d_e:
		data->nr_bank = ARRAY_SIZE(nct6796d_e_gpio_bank);
		data->bank = nct6796d_e_gpio_bank;

                /* enable GPIO.It seems no side-effect if we enable all of them */
		superio_select(sio->addr, 7);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0x0F;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);

		superio_select(sio->addr, 9);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0x0F;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);
		break; 

	case nct6791d:
		data->nr_bank = ARRAY_SIZE(nct6791d_gpio_bank);
		data->bank = nct6791d_gpio_bank;

                /* enable GPIO.It seems no side-effect if we enable all of them */
		superio_select(sio->addr, 7);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0x07;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);

		superio_select(sio->addr, 9);
		gpio_cfg = superio_inb(sio->addr, SIO_GPIO_ENABLE);
		gpio_cfg |= 0x0F;
		superio_outb(sio->addr, SIO_GPIO_ENABLE, gpio_cfg);
		break;
   
	default:
		superio_exit(sio->addr);
		return -ENODEV;
	}


	if (sw_irq_enable){
		pr_info(DRVNAME ": SW_IRQ enabled (poll_rate=%dms)\n", polling_rate);
		timer = devm_kzalloc(&pdev->dev, sizeof(*timer), GFP_KERNEL);
		if (!timer) {
			err = -ENOMEM;
			goto err_gpiochip;
		}
		/* Setup the timer */
		timer_setup(timer, timer_handler, 0);
	}
	
	data->sio = sio;

	platform_set_drvdata(pdev, data);

	/* For each GPIO bank, register a GPIO chip. */
	for (i = 0; i < data->nr_bank; i++) {
		struct _nct5104d_gpio_bank *bank = &data->bank[i];
		if (sw_irq_enable){
			bank->chip.to_irq = gpio_nct5104d_to_irq;
			bank->sw_irq_provider.irq_chip.irq_unmask= nct5104d_irq_unmask;
			bank->sw_irq_provider.irq_chip.irq_mask = nct5104d_irq_mask;
			bank->sw_irq_provider.irq_chip.irq_set_type = nct5104d_irq_set_type;
			bank->sw_irq_provider.irq_chip.irq_set_affinity = nct5104d_irq_set_affinity;
			bank->sw_irq_provider.irq_num = bank->chip.ngpio;
			bank->sw_irq_provider.platform_dev = pdev;
			bank->sw_irq_provider.irq_mask_reg = UINT_MAX;
			bank->sw_irq_provider.irq_rising_reg = 0;
			bank->sw_irq_provider.irq_falling_reg = 0;
			// we change irq_alloc_descs to devm_irq_alloc_descs from kernel 4.11
			// bank->sw_irq_provider.irq_base = irq_alloc_descs(-1, 0, bank->sw_irq_provider.irq_num, 0);
			bank->sw_irq_provider.irq_base = devm_irq_alloc_descs(&pdev->dev, -1, 0, bank->sw_irq_provider.irq_num, -1);
			if(bank->sw_irq_provider.irq_base < 0){
				pr_err("%s:%d err=%d\n", __FUNCTION__, __LINE__, err);
				goto err_gpiochip;
			}

			bank->sw_irq_provider.irq_domain = irq_domain_add_legacy(NULL, bank->sw_irq_provider.irq_num, bank->sw_irq_provider.irq_base, 0, &nct5104d_irq_ops, &bank->sw_irq_provider);
			if (!bank->sw_irq_provider.irq_domain)
			{
				err = -ENODEV;
				pr_err("%s:%d err=%d\n", __FUNCTION__, __LINE__, err);
				goto err_gpiochip;
			}
			spin_lock_init(&bank->sw_irq_provider.lock);
		}

		bank->chip.parent = &pdev->dev;
		bank->data = data;

		// use devm_gpiochip_add_data, we does not gpiochip_remove anymore
		//err = gpiochip_add(&bank->chip);
		err = devm_gpiochip_add_data(&pdev->dev, &bank->chip, bank);
		if (err) {
			dev_err(&pdev->dev,
				"Failed to register gpiochip %d: %d\n",
				i, err);
			goto err_gpiochip;
		}
	}

	superio_exit(sio->addr);

	g_nct5104d_gpio_data = data;
	return 0;

err_gpiochip:
	if(timer) del_timer_sync(timer);
	
	for (i = i - 1; i >= 0; i--) {
		struct _nct5104d_gpio_bank *bank = &data->bank[i];
		if (bank->sw_irq_provider.irq_domain)irq_domain_remove(bank->sw_irq_provider.irq_domain);
		//irq_free_descs(bank->sw_irq_provider.irq_base, bank->sw_irq_provider.irq_num);	//we change to devm_irq_alloc_descs and we does not irq_free_descs
	}

	superio_exit(sio->addr);
	return err;
}

static int nct5104d_gpio_remove(struct platform_device *pdev)
{
	int i;
	struct _nct5104d_gpio_data *data = platform_get_drvdata(pdev);

	if (sw_irq_enable){
		if(timer) del_timer_sync(timer);
	}

	for (i = 0; i < data->nr_bank; i++) {
		struct _nct5104d_gpio_bank *bank = &data->bank[i];
		if (sw_irq_enable) {
			if (bank->sw_irq_provider.irq_domain)irq_domain_remove(bank->sw_irq_provider.irq_domain);
			//irq_free_descs(bank->sw_irq_provider.irq_base, bank->sw_irq_provider.irq_num);	//we change to devm_irq_alloc_descs and we does not irq_free_descs
		}

	}

	return 0;
}

static int __init nct5104d_find(int addr, struct _nct5104d_sio *sio)
{
	int err;
	u16 devid;


	err = superio_enter(addr);
	if (err)
		return err;

	err = -ENODEV;

	devid = superio_inw(addr, SIO_CHIPID) & SIO_ID_MASK;

	switch (devid) {
	case SIO_NCT5104D_ID:
		sio->type = nct5104d;
		break;
	case SIO_NCT5124D_ID:
		sio->type = nct5124d;
		break;
	case SIO_NCT6106D_ID:
	case SIO_NCT6126D_ID:
		sio->type = nct6106d;
		break;
	case SIO_NCT6796D_E_ID:
        	sio->type = nct6796d_e;
        	break;
	case SIO_NCT6791D_ID:
        	sio->type = nct6791d;
        	break;
	default:
		pr_info(DRVNAME ": Unsupported device 0x%04x\n", devid);
		goto err;
	}
	sio->addr = addr;
	err = 0;



	pr_info(DRVNAME ": Found %s at %#x chip id 0x%04x\n",
		nct5104d_names[sio->type],
		(unsigned int) addr,
		(int) superio_inw(addr, SIO_CHIPID));

		/* Only configure GPIO0/GPIO1 for backward-compatible for last driver.
		 * We don't configure others and let it be configured by BIOS etc. (by SYHuang)
		 */
        superio_select(sio->addr, SIO_LD_GPIO_MODE);
        superio_outb(sio->addr, SIO_GPIO0_MODE, 0x0);
        superio_outb(sio->addr, SIO_GPIO1_MODE, 0x0);

err:
	superio_exit(addr);
	return err;
}

static struct platform_device *nct5104d_gpio_pdev;

static int __init
nct5104d_gpio_device_add(const struct _nct5104d_sio *sio)
{
	int err;

	nct5104d_gpio_pdev = platform_device_alloc(DRVNAME, -1);
	if (!nct5104d_gpio_pdev)
		pr_err(DRVNAME ": Error platform_device_alloc\n");
	if (!nct5104d_gpio_pdev)
		return -ENOMEM;

	err = platform_device_add_data(nct5104d_gpio_pdev,
				       sio, sizeof(*sio));
	if (err) {
		pr_err(DRVNAME "Platform data allocation failed\n");
		goto err;
	}

	err = platform_device_add(nct5104d_gpio_pdev);
	if (err) {
		pr_err(DRVNAME "Device addition failed\n");
		goto err;
	}
	pr_info(DRVNAME ": Device added\n");
	return 0;

err:
	platform_device_put(nct5104d_gpio_pdev);

	return err;
}

/*
 */

static struct platform_driver nct5104d_gpio_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DRVNAME,
	},
	.probe		= nct5104d_gpio_probe,
	.remove		= nct5104d_gpio_remove,
};

static int __init nct5104d_gpio_init(void)
{
	int err;
	struct _nct5104d_sio sio;


	if (nct5104d_find(0x2e, &sio) &&
	    nct5104d_find(0x4e, &sio))
		return -ENODEV;


#if 0
	pr_info("nct5104d_gpio_init(): sio.addr=%X\r\n",(u32)sio.addr);
#endif
	err = platform_driver_register(&nct5104d_gpio_driver);
	if (!err) {
		pr_info(DRVNAME ": platform_driver_register\n");
		err = nct5104d_gpio_device_add(&sio);
		if (err)
			platform_driver_unregister(&nct5104d_gpio_driver);
	}

	return err;
}
subsys_initcall(nct5104d_gpio_init);

static void __exit nct5104d_gpio_exit(void)
{
	platform_device_unregister(nct5104d_gpio_pdev);
	platform_driver_unregister(&nct5104d_gpio_driver);
}
module_exit(nct5104d_gpio_exit);

MODULE_DESCRIPTION("GPIO driver for Super-I/O chips NCT5104D/NCT6106D");
MODULE_VERSION(ADVANTECH_NCT_GPIO_VER);
MODULE_AUTHOR("Tasanakorn Phaipool <tasanakorn@gmail.com>");
MODULE_AUTHOR("Chic Lee <chic.lee@advantech.com.tw>");
MODULE_LICENSE("GPL");

