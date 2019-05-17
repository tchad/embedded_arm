#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <linux/gpio.h>


static __u32 major_number;
static dev_t dev_num = 0;
struct device *dev;
static struct class *dev_class;
static struct cdev dev_cdev;

#define IOCTL_LED_OFF 0
#define IOCTL_LED_ON 1

#define GPIO_C_BASE (2*32)

static const unsigned int GPIOC31_ID = GPIO_C_BASE + 31;
#define GPIO_LED_PIN GPIOC31_ID

static const unsigned int GPIOC29_ID = GPIO_C_BASE + 29;
#define GPIO_BTN_PIN GPIOC29_ID

#define GPIO_LOW 0
#define GPIO_HIGH 1


static int gpio_port_init(void)
{
	int err;
	err = gpio_request(GPIO_LED_PIN, "gpio_hello_led");
	if(err < 0) {
		pr_err("GPIO_HELLO: Error requesting LED PIN");
		return err;
	}

	err = gpio_direction_output(GPIO_LED_PIN, 1);
	if(err < 0) {
		pr_err("GPIO_HELLO: Error setting direction output on LED PIN");
		return err;
	}

	err = gpio_request(GPIO_BTN_PIN, "gpio_hello_btn");
	if(err < 0) {
		pr_err("GPIO_HELLO: Error requesting BTN PIN");
		return err;
	}

	err = gpio_direction_input(GPIO_BTN_PIN);
	if(err < 0) {
		pr_err("GPIO_HELLO: Error setting direction input on BTN PIN");
		return err;
	}

	gpio_set_value(GPIO_LED_PIN, GPIO_LOW);

	return 0;
}

static void gpio_port_close(void)
{
	gpio_set_value(GPIO_LED_PIN, GPIO_LOW);
	gpio_free(GPIO_LED_PIN);
}

static int gpio_led_ctrl(unsigned int setting)
{
	gpio_set_value(GPIO_LED_PIN, setting);
	return 0;
}

static int gpio_btn_status(void) 
{
	int data = gpio_get_value(GPIO_BTN_PIN);
	return data;
}


static ssize_t gpio_read (struct file *f, char __user *uspace_buffer, size_t read_size, loff_t *pos) 
{
	int gpio_btn_data = 0;
	size_t max_buff_size;
	unsigned long bytes_to_copy;
	char *src;
	unsigned long err;

	max_buff_size = sizeof(gpio_btn_data);

	bytes_to_copy = read_size;
	if( read_size > max_buff_size) {
		bytes_to_copy = max_buff_size;
	}

	gpio_btn_data = gpio_btn_status();

	src = (char*)(&gpio_btn_data);
	err = copy_to_user(uspace_buffer, src, bytes_to_copy);
	if(err) {
		return -EFAULT;
	}

	return bytes_to_copy;
}


static long gpio_unlocked_ioctl (struct file *f, unsigned int command, unsigned long arg) 
{
	switch(command) {
		case 0:
			pr_info("GPIO_HELLO: Got IOCTL LED OFF\n");
			gpio_led_ctrl(GPIO_LOW);
			break;
		case 1:
			pr_info("GPIO_HELLO: Got IOCTL LED ON\n");
			gpio_led_ctrl(GPIO_HIGH);
			break;
		default:
			return -ENOTTY;
	}

	return 0;
}

static ssize_t gpio_write (struct file *f, const char __user *u, size_t s, loff_t *o)
{
	return 0;
}

static int gpio_open (struct inode *i, struct file *f)
{
	pr_info("GPIO_HELLO: GPIO DRV open");
	return 0;
}

static int gpio_release (struct inode *i, struct file *f)
{
	pr_info("GPIO_HELLO: GPIO DRV release");
	return 0;
}

static struct file_operations gpio_fops = {
	.read = gpio_read,
	.write = gpio_write,
	.open = gpio_open,
	.release = gpio_release,
	.unlocked_ioctl = gpio_unlocked_ioctl
};


static int __init gpio_drv_init(void)
{
	int err;

	pr_info("GPIO_HELLO: Initializing HELLO GPIO\n");
	err = gpio_port_init();
	if(err < 0) {
		return err;
	}

	/* Obtain major number  */
	err = alloc_chrdev_region(&dev_num, 0, 1, "gpio_hello");
	if(err < 0) {
		pr_err("GPIO_HELLO: Unable to obtain device major number");
		return err;
	}

	major_number = MAJOR(dev_num);

	/* Create device class in /sys/class */
	dev_class = class_create(THIS_MODULE, "gpio_hello_class");
	if(IS_ERR(dev_class)) {
		pr_err("GPIO_HELLO: Unable to create class for gpio_hello");
		unregister_chrdev_region(MKDEV(major_number, 0), 1);
		return PTR_ERR(dev_class);
	}

	/* Initialize device file */
	cdev_init(&dev_cdev, &gpio_fops);
	dev_cdev.owner = THIS_MODULE;
	cdev_add(&dev_cdev, dev_num, 1);

	dev = device_create(dev_class,
			    NULL, /* no parent */
			    dev_num,
			    NULL, /* no additional data */
			    "gpio_hello" /* name */
			);

	if(IS_ERR(dev)) {
		pr_err("GPIO_HELLO: Unable to create device for gpio_hello");
		class_destroy(dev_class);
		unregister_chrdev_region(dev_num, 1);
		return PTR_ERR(dev);
	}
			
	pr_info("GPIO_HELLO: gpio_hello loaded");
	return 0;
}

static void __exit gpio_drv_clean(void)
{
	unregister_chrdev_region(MKDEV(major_number, 0), 1);
	device_destroy(dev_class, MKDEV(major_number, 0));
	cdev_del(&dev_cdev);
	class_destroy(dev_class);

	gpio_port_close();

	pr_info("GPIO_HELLO: gpio_hello unloaded");
}

module_init(gpio_drv_init);
module_exit(gpio_drv_clean);

MODULE_AUTHOR("Tomasz Chadzynski <tomasz.chadzynski@sjsu.edu");
MODULE_DESCRIPTION("HW1 not graded");
MODULE_LICENSE("Proprietary");




/*
static ssize_t gpio_read_unused (struct file *f, char __user *uspace_buffer, size_t read_size, loff_t *pos) 
{
	static unsigned dummy_data = 0;
	size_t max_buff_size;
	loff_t buff_end;
	unsigned long bytes_to_copy;
	char *src;
	unsigned long err;


	max_buff_size = sizeof(dummy_data);
	buff_end = sizeof(dummy_data);
	if(*pos >= buff_end) {
		return 0;
	}

	bytes_to_copy = read_size;
	if((*pos + read_size) > buff_end) {
		bytes_to_copy = buff_end - *pos;
	}


	src = &dummy_data + *pos;
	err = copy_to_user(uspace_buffer, src, bytes_to_copy);
	if(err) {
		return -EFAULT;
	}


	*pos += bytes_to_copy;

	if(dummy_data == 0) {
		dummy_data = 1;
	} else {
		dummy_data = 0;
	}

	return bytes_to_copy;
}
*/
