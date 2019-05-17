#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>

#define CRA_REG_M 0x0
#define CRB_REG_M 0x1
#define MR_REG_M 0x2
#define OUT_X_H_M 0x3
#define OUT_X_L_M 0x4
#define OUT_Z_H_M 0x5
#define OUT_Z_L_M 0x6
#define OUT_Y_H_M 0x7
#define OUT_Y_L_M 0x8
#define SR_REG_M 0x9
#define IRA_REG_M 0x0A

#define SR_REG_M_DRDY_MASK 0x1
#define SR_REG_M_LOCK_MASK 0x2

#define MAG_GAIN_1_3 0x1 << 5
#define MAG_GAIN_1_9 0x2 << 5
#define MAG_GAIN_2_5 0x3 << 5
#define MAG_GAIN_4_0 0x4 << 5
#define MAG_GAIN_4_7 0x5 << 5
#define MAG_GAIN_5_6 0x6 << 5
#define MAG_GAIN_8_1 0x7 << 5

#define MAG_DRATE_HZ_0_75 0x0 << 2
#define MAG_DRATE_HZ_1_5 0x1 << 2
#define MAG_DRATE_HZ_3_0 0x2 << 2
#define MAG_DRATE_HZ_7_5 0x3 << 2
#define MAG_DRATE_HZ_15_0 0x4 << 2
#define MAG_DRATE_HZ_30_0 0x5 << 2
#define MAG_DRATE_HZ_75_0 0x6 << 2
#define MAG_DRATE_HZ_220_0 0x7 << 2

#define DEV_CHECK_REG_ADDR IRA_REG_M
#define	DEV_CHECK_REG_EXPVAL 0x48


struct raw_mag_read {
	s16 x;
	s16 y;
	s16 z;
};

struct lsm303_mag_device {
	struct i2c_client *client;
	struct mutex mutex;
	int curr_file_ptr;
	struct raw_mag_read raw_data;
	int raw_mag_read_new;
	struct task_struct *polling_thread;

	struct cdev cdev;

};

static dev_t dev_num;
static u32 major_number;
static struct class *dev_class;
static struct device *dev;

static int read_reg(struct lsm303_mag_device *dev, u8 reg_addr, u8 *buf)
{
	int err;
	struct i2c_msg msg[2];

	msg[0].addr = dev->client->addr;
	msg[0].flags = 0x0;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = dev->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	err = i2c_transfer(dev->client->adapter, msg, 2);

	return err;
}

static int write_reg(struct lsm303_mag_device *dev, u8 reg_addr, u8 data)
{
	int err;
	struct i2c_msg msg;
	u8 buf[2];

	buf[0] = reg_addr;
	buf[1] = data;

	msg.addr = dev->client->addr;
	msg.flags = 0x0;
	msg.len = 2;
	msg.buf = buf;

	err = i2c_transfer(dev->client->adapter, &msg, 1);

	return err;
}

static int read_reg_n(struct lsm303_mag_device *dev, u8 reg_addr, u8 *buf, int n)
{
	int err;
	struct i2c_msg msg[2];

	msg[0].addr = dev->client->addr;
	msg[0].flags = 0x0;
	msg[0].len = 1;
	msg[0].buf = &reg_addr;

	msg[1].addr = dev->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = n;
	msg[1].buf = buf;

	err = i2c_transfer(dev->client->adapter, msg, 2);

	return err;
}

static int i2c_device_verify(struct lsm303_mag_device *dev)
{
	int err;
	u8 data;

	err = read_reg(dev, DEV_CHECK_REG_ADDR, &data);

	if (err >= 0 && data == DEV_CHECK_REG_EXPVAL) {
		return 0;
	}
	else {
		return -1;
	}
}

static int mag_data_poll_thread_fn(void *data)
{
	struct lsm303_mag_device *dev_data;
	int err;
	u8 status;
	int skip_read;

	u8 raw_buffer[6];
	dev_data = (struct lsm303_mag_device*)data;

	pr_info("MAG_SENSOR: thread started\n");

	while(!kthread_should_stop()) {
		err = read_reg(dev_data, SR_REG_M, &status);
		if(err < 0) {
			pr_err("MAG_SENSOR: Failed to read mgnetometer status\n");
		}
		else {
			if(status & SR_REG_M_DRDY_MASK) {
				skip_read = 0;

				err = read_reg_n(dev_data, OUT_X_H_M, raw_buffer, 6);
				if(err < 0) skip_read = 1;
				

				if(skip_read) {
					pr_err("MAG_SENSOR: error reading sensor data\n");
				}
				else {
					struct raw_mag_read tmp;
					tmp.x = raw_buffer[0] << 8 | raw_buffer[1];
					tmp.y = raw_buffer[4] << 8 | raw_buffer[5];
					tmp.z = raw_buffer[2] << 8 | raw_buffer[3];

					mutex_lock_killable(&dev_data->mutex);
					dev_data->raw_data = tmp;
					dev_data->raw_mag_read_new = 1;
					mutex_unlock(&dev_data->mutex);
				}
			}
		}

		schedule();
	}

	pr_info("MAG_SENSOR: thread stopped\n");

	do_exit(0);
}


static int open(struct inode *i, struct file *f) 
{
    struct lsm303_mag_device* dev_data;
    dev_data = container_of(i->i_cdev, struct lsm303_mag_device, cdev);

    f->private_data = dev_data;
	return 0;
}

static int release(struct inode *i, struct file *f)
{
	return 0;
}

static ssize_t read(struct file *f, char __user *buf, size_t count, loff_t *f_pos)
{
	struct lsm303_mag_device *dev_data;
    struct raw_mag_read data;
    unsigned long bytes_to_copy;
    char *src;
    dev_data = f->private_data;

    while(1) {
        mutex_lock_killable(&dev_data->mutex);
        if(dev_data->raw_mag_read_new != 0) {
            data = dev_data->raw_data;
            dev_data->raw_mag_read_new = 0;
            mutex_unlock(&dev_data->mutex);
            break;
        } else {
            mutex_unlock(&dev_data->mutex);
            schedule();
        }
    }

    bytes_to_copy = sizeof(struct raw_mag_read);
    if(count < bytes_to_copy) {
        bytes_to_copy = count;
    }

    src = (char*)&data;
    copy_to_user(buf, src, bytes_to_copy);

    return bytes_to_copy;

}

static struct file_operations lsm303_fops = {
	.owner =    THIS_MODULE,
	.read =     read,
	.open =     open,
	.release =  release,
};


static int probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	struct lsm303_mag_device *dev_data;

	dev_data = (struct lsm303_mag_device*)kzalloc(sizeof(struct lsm303_mag_device), GFP_KERNEL);
	dev_data->client = client;
	mutex_init(&dev_data->mutex);

	i2c_set_clientdata(client, dev_data);

	err = i2c_device_verify(dev_data);

	err = write_reg(dev_data, MR_REG_M, 0x00);
	if(err < 0) pr_err("MAG_SENSOR: Can't enable\n");

	err = write_reg(dev_data, CRB_REG_M, MAG_GAIN_1_3);
	if(err < 0) pr_err("MAG_SENSOR: Can't set gain\n");

	err = write_reg(dev_data, CRA_REG_M, MAG_DRATE_HZ_75_0);
	if(err < 0) pr_err("MAG_SENSOR: Can't set freq\n");

	dev_data->polling_thread = kthread_create(mag_data_poll_thread_fn, dev_data, "mag_poll_thread");
	wake_up_process(dev_data->polling_thread);

    err = alloc_chrdev_region(&dev_num, 0, 1, "lsm303_mag");
    major_number = MAJOR(dev_num);
    dev_class = class_create(THIS_MODULE, "lsm303_mag_class");
    cdev_init(&dev_data->cdev, &lsm303_fops);
    dev_data->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev_data->cdev, dev_num, 1);
    dev = device_create(dev_class, NULL, dev_num, NULL, "lsm303_mag");

	return 0;
}

static int remove(struct i2c_client *client)
{
	struct lsm303_mag_device *dev_data;

	pr_info("MAG_SENSOR: Removing driver\n");

	dev_data = i2c_get_clientdata(client);

    device_destroy(dev_class, MKDEV(major_number, 0));
    class_destroy(dev_class);
    unregister_chrdev_region(MKDEV(major_number, 0), 1);
    cdev_del(&dev_data->cdev);

	kthread_stop(dev_data->polling_thread);

	mutex_destroy(&dev_data->mutex);
	kfree(dev_data);

	return 0;
}


static const struct of_device_id lsm303dlhc_ids[] = {
	{ .compatible = "stm,lsm303dlhc_mag", },
	{ /* sentinel */ }
};

static const struct i2c_device_id lsm303dlhc_id[] = {
	{"lsm303dlhc_mag", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, lsm303dlhc_id);


static struct i2c_driver lsm303dlhc_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "lsm303dlhc_mag",
		.of_match_table = of_match_ptr(lsm303dlhc_ids),
	},
	.probe = probe,
	.remove = remove,
	.id_table = lsm303dlhc_id,
};

module_i2c_driver(lsm303dlhc_i2c_driver);

MODULE_AUTHOR("Tomasz Chadzynski <tomasz.chadzynski@sjsu.edu>");
MODULE_DESCRIPTION("Compass module lsm303dlhc driver, EE242 project 2");
MODULE_LICENSE("GPL");
