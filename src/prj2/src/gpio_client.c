
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include "gpio_client.h"

void gpio_read_btn_state(gpio_dev *dev, btn_state *state)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:gpio_read_btn_state: Null pointer to dev.\n");
		return;
	}

	unsigned int tmp;
	read(dev->gpio_fd, &tmp, sizeof(tmp));
	if(tmp) {
		*state = BTN_PRESSED;
	} else {
		*state = BTN_RELEASED;
	}
}

void gpio_enable_led(gpio_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:gpio_enable_led: Null pointer to dev.\n");
		return;
	}

	ioctl(dev->gpio_fd, IOCTL_CMD_LED_ON);
}

void gpio_disable_led(gpio_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:gpio_disable_led: Null pointer to dev.\n");
		return;
	}

	ioctl(dev->gpio_fd, IOCTL_CMD_LED_OFF);
}

int gpio_dev_open(gpio_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:gpio_dev_open: Null pointer to dev.\n");
		return -1;
	}

	dev->gpio_fd = open(GPIO_PATH, O_RDWR);

	if(dev->gpio_fd < 0) {
		syslog(LOG_ERR, "EE242_PRJ2:gpio_dev_open: Failed to open gpio_hello.\n");
		return  -1;
	}

	return 0;
}

void gpio_dev_close(gpio_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:gpio_dev_close: Null pointer to dev.\n");
		return;
	}

	close(dev->gpio_fd);
}
