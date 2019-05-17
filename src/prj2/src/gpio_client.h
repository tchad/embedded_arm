#ifndef __GPIO_CLIENT_H__
#define __GPIO_CLIENT_H__

#define IOCTL_CMD_LED_OFF 0
#define IOCTL_CMD_LED_ON 1

#define GPIO_PATH "/dev/gpio_hello"

typedef enum _btn_state {
	BTN_PRESSED,
	BTN_RELEASED
} btn_state;

typedef struct _gpio_dev {
	int gpio_fd;
} gpio_dev;


void gpio_read_btn_state(gpio_dev *dev, btn_state *state);
void gpio_enable_led(gpio_dev *dev);
void gpio_disable_led(gpio_dev *dev);

int gpio_dev_open(gpio_dev *dev);
void gpio_dev_close(gpio_dev *dev);

#endif
