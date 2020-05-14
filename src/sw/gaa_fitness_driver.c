/*
 * gaa_fitness_driver.c
 *
 * Device driver for GAA Fitness Peripheral
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "gaa_fitness_driver.h"

#define DRIVER_NAME "gaa_fitness"

/* Device Registers */
#define P1(x)      (x)
#define P2(x)      ((x)+2)
#define P1XORP2(x) ((x)+4)

/*
 * Information about our device
 */
struct gaa_fitness_dev {
	struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
    gaa_fitness_inputs_t inputs;
	gaa_fitness_outputs_t outputs;
} dev;

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_inputs(gaa_fitness_inputs_t *i) {

	iowrite8(i->p1, P1(dev.virtbase) );
	iowrite8(i->p2, P2(dev.virtbase) );
	dev.inputs = *i;
}

static void read_outputs(void) {

    dev.outputs.p1xorp2 = ioread8(P1XORP2(dev.virtbase));
}

/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long gaa_fitness_ioctl(struct file *f, 
                              unsigned int cmd, 
                              unsigned long arg) {

	gaa_fitness_arg_t gaa_arg;

	switch (cmd) {
	case GAA_FITNESS_WRITE_INPUTS:
		if (copy_from_user(&gaa_arg, (gaa_fitness_arg_t *) arg,
				   sizeof(gaa_fitness_arg_t)))
			return -EACCES;
		write_inputs(&gaa_arg.inputs);
		break;

	case GAA_FITNESS_READ_OUTPUTS:
        read_outputs();
	  	gaa_arg.outputs = dev.outputs;
		if (copy_to_user((gaa_fitness_arg_t *) arg, &gaa_arg,
				 sizeof(gaa_fitness_arg_t)))
			return -EACCES;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* The operations our device knows how to do */
static const struct file_operations gaa_fitness_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = gaa_fitness_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice gaa_fitness_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &gaa_fitness_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init gaa_fitness_probe(struct platform_device *pdev) {

    //vga_ball_color_t beige = { 0xf9, 0xe4, 0xb7 };
	//vga_ball_position_t top_left_corner = { 0x04, 0x04 };
	int ret;

	/* Register ourselves as a misc device: creates /dev/vga_ball */
	ret = misc_register(&gaa_fitness_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}

	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}

	/* Set an initial color and position */
    //write_background(&beige);
	//write_position(&top_left_corner);

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&gaa_fitness_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int gaa_fitness_remove(struct platform_device *pdev) {
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&gaa_fitness_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id gaa_fitness_of_match[] = {
	{ .compatible = "wgs2113_jcr2198,gaa_fitness-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, gaa_fitness_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver gaa_fitness_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(gaa_fitness_of_match),
	},
	.remove	= __exit_p(gaa_fitness_remove),
};

/* Called when the module is loaded: set things up */
static int __init gaa_fitness_init(void) 
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&gaa_fitness_driver, gaa_fitness_probe);
}

/* Calball when the module is unloaded: release resources */
static void __exit gaa_fitness_exit(void) {

	platform_driver_unregister(&gaa_fitness_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(gaa_fitness_init);
module_exit(gaa_fitness_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Graham Stubbs (wgs2113) and Jarrett Ross (jcr2198)");
MODULE_DESCRIPTION("GAA Fitness Driver");

