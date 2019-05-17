#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <linux/gpio.h>

#include <linux/pwm.h>

#define IOCTL_MOTOR_EN 0
#define IOCTL_MOTOR_CFG 1

#define PWM_MOTOR_PIN_ID 0

struct motor_cfg {
	int direction;
	int period_ns;
	int duty_ns;
};

/*
 * Current state of the device configuration
 */
struct motor_state_desc {
	int enabled;
	struct motor_cfg cfg;
};


#define GPIO_C_BASE (2*32)
static const unsigned int GPIOC13_ID = GPIO_C_BASE + 13;
#define GPIO_DIR_PIN GPIOC13_ID

#define GPIO_LOW 0
#define GPIO_HIGH 1

static __u32 major_number;
static dev_t dev_num = 0;
static struct class *dev_class;
static struct cdev dev_cdev;
static struct device *dev;
static struct pwm_device *pwm0_dev;
static struct motor_state_desc motor_state;


static void motor_set_default(void){
	pwm_disable(pwm0_dev);
	motor_state.enabled = 0;
	motor_state.cfg.direction = 0;
	motor_state.cfg.duty_ns = 0;
	motor_state.cfg.period_ns = 0;
}

static void motor_set(struct motor_cfg cfg, int enabled){
	int now_enabled;
	int new_enabled;
	if(motor_state.enabled == 1 && motor_state.cfg.direction != 0){
		now_enabled = 1;
	} else {
		now_enabled = 0;
	}

	if(enabled == 1 && cfg.direction != 0){
		new_enabled = 1;
	} else {
		new_enabled = 0;
	}

	if(now_enabled == 1 && new_enabled == 0) {
		pwm_disable(pwm0_dev);
	}
	else if(now_enabled == 0 && new_enabled == 1) {
		pwm_enable(pwm0_dev);
	}

	if(cfg.direction < 0) {
		gpio_set_value(GPIO_DIR_PIN, GPIO_LOW);
	} else {
		gpio_set_value(GPIO_DIR_PIN, GPIO_HIGH);
	}

	if(cfg.duty_ns != motor_state.cfg.duty_ns) {
		pwm_config(pwm0_dev, cfg.duty_ns, cfg.period_ns);
	}

	motor_state.enabled = enabled;
	motor_state.cfg = cfg;
}


static ssize_t prj1_chrdev_read (struct file *f, char __user *uspace_buffer, size_t read_size, loff_t *pos) 
{
	size_t max_buff_size;
	unsigned long bytes_to_copy;
	char *src;
	unsigned long err;

	max_buff_size = sizeof(motor_state);

	bytes_to_copy = read_size;
	if( read_size > max_buff_size) {
		bytes_to_copy = max_buff_size;
	}

	src = (char*)(&motor_state);
	err = copy_to_user(uspace_buffer, src, bytes_to_copy);
	if(err) {
		return -EFAULT;
	}

	return bytes_to_copy;
}


static long prj1_unlocked_ioctl (struct file *f, unsigned int command, unsigned long arg) 
{
	struct motor_cfg new_cfg = motor_state.cfg;
	int new_enabled = motor_state.enabled;
	switch(command) {
		case IOCTL_MOTOR_EN:
			copy_from_user(&new_enabled, (char*)arg, sizeof(int));
			motor_set(new_cfg, new_enabled);
			break;
		case IOCTL_MOTOR_CFG:
			copy_from_user(&new_cfg, (char*)arg, sizeof(struct motor_cfg));
			motor_set(new_cfg, new_enabled);
			break;
		default:
			return -ENOTTY;
	}

	return 0;
}

static ssize_t prj1_write (struct file *f, const char __user *u, size_t s, loff_t *o)
{
	return 0;
}

static int prj1_open (struct inode *i, struct file *f)
{
	pr_info("MOTOR_CTRL: DRV open");
	return 0;
}

static int prj1_release (struct inode *i, struct file *f)
{
	pr_info("MOTOR_CTRL: DRV release");
	return 0;
}

static struct file_operations prj1_fops = {
	.read = prj1_chrdev_read,
	.write = prj1_write,
	.open = prj1_open,
	.release = prj1_release,
	.unlocked_ioctl = prj1_unlocked_ioctl
};

static int __init prj1_drv_init(void)
{
	int err;

	pr_info("MOTOR_CTRL: Initializing DC motor controller\n");

	/* Obtain major number  */
	err = alloc_chrdev_region(&dev_num, 0, 1, "motor_ctrl");
	if(err < 0) {
		pr_err("MOTOR_CTRL: Unable to obtain device major number");
		return err;
	}

	major_number = MAJOR(dev_num);

	/* Create device class in /sys/class */
	dev_class = class_create(THIS_MODULE, "motor_ctrl_class");
	if(IS_ERR(dev_class)) {
		pr_err("MOTOR_CTRL: Unable to create class for motor_ctrl");
		unregister_chrdev_region(MKDEV(major_number, 0), 1);
		return PTR_ERR(dev_class);
	}

	/* Initialize device file */
	cdev_init(&dev_cdev, &prj1_fops);
	dev_cdev.owner = THIS_MODULE;
	cdev_add(&dev_cdev, dev_num, 1);

	dev = device_create(dev_class,
			    NULL, /* no parent */
			    dev_num,
			    NULL, /* no additional data */
			    "motor_ctrl" /* name */
			);

	if(IS_ERR(dev)) {
		pr_err("MOTOR_CTRL: Unable to create device for motor_ctrl");
		class_destroy(dev_class);
		unregister_chrdev_region(dev_num, 1);
		return PTR_ERR(dev);
	}

	/* Initialize GPIO port for the direction signal */
	err = gpio_request(GPIO_DIR_PIN, "motor_ctrl_direction");
	if(err < 0) {
		pr_err("MOTOR_CTRL: Error requesting DIRECTION PIN");
		device_destroy(dev_class, dev_num);
		class_destroy(dev_class);
		unregister_chrdev_region(MKDEV(major_number, 0), 1);
		cdev_del(&dev_cdev);
		return err;
	}

	err = gpio_direction_output(GPIO_DIR_PIN, 1);
	if(err < 0) {
		pr_err("MOTOR_CTRL: Error setting direction output on DIRECTION PIN");
		gpio_free(GPIO_DIR_PIN);
		device_destroy(dev_class, dev_num);
		class_destroy(dev_class);
		unregister_chrdev_region(MKDEV(major_number, 0), 1);
		cdev_del(&dev_cdev);
		return err;
	}

	gpio_set_value(GPIO_DIR_PIN, GPIO_LOW);

	/* Obtain pwm 0 pin */
	pwm0_dev = pwm_request(PWM_MOTOR_PIN_ID, "pwm0");
	if(IS_ERR(pwm0_dev)){
		pr_err("MOTOR_CTRL: unable to obtain pwm0_pin");
		gpio_free(GPIO_DIR_PIN);
		device_destroy(dev_class, dev_num);
		class_destroy(dev_class);
		unregister_chrdev_region(MKDEV(major_number, 0), 1);
		cdev_del(&dev_cdev);
		return PTR_ERR(pwm0_dev);
	}

	/* Set motor in default */
	motor_state.enabled = 0;
	motor_state.cfg.direction = 0;

	motor_set_default();
			
	pr_info("MOTOR_CTRL: driver loaded");
	return 0;
}

static void __exit prj1_drv_clean(void)
{
	/* Set motor in default */
	motor_state.enabled = 0;
	motor_state.cfg.direction = 0;

	motor_set_default();

	pwm_free(pwm0_dev);

	gpio_set_value(GPIO_DIR_PIN, GPIO_LOW);
	gpio_free(GPIO_DIR_PIN);

	device_destroy(dev_class, MKDEV(major_number, 0));
	class_destroy(dev_class);
	unregister_chrdev_region(MKDEV(major_number, 0), 1);
	cdev_del(&dev_cdev);

	pr_info("MOTOR_CTRL: driver unloaded");
}

module_init(prj1_drv_init);
module_exit(prj1_drv_clean);

MODULE_AUTHOR("Tomasz Chadzynski <tomasz.chadzynski@sjsu.edu");
MODULE_DESCRIPTION("DC motor controller");
MODULE_LICENSE("Proprietary");



