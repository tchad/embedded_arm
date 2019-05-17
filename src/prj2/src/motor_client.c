#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <stdio.h>
#include "motor_client.h"

int motor_read(motor_dev *dev, motor_state_desc* buff)
{
	ssize_t bytes_read;

	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:motor_read: Null pointer to dev.\n");
		return -1;
	}

	lseek(dev->motor_fd, 0, SEEK_SET);
	bytes_read = read(dev->motor_fd, buff, sizeof(motor_state_desc));
	
	return 0;
}
int motor_enable(motor_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:motor_enable: Null pointer to dev.\n");
		return -1;
	}
	int en = 1;

	ioctl(dev->motor_fd, IOCTL_MOTOR_EN, &en);

	return 0;
}

int motor_disable(motor_dev *dev) {
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:motor_disable: Null pointer to dev.\n");
		return -1;
	}
	int en = 0;

	ioctl(dev->motor_fd, IOCTL_MOTOR_EN, &en);

	return 0;
}

void motor_set_cfg(motor_dev *dev, motor_cfg cfg)
{
	ioctl(dev->motor_fd, IOCTL_MOTOR_CFG, &cfg);
}

int motor_dev_open(motor_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:motor_dev_open: Null pointer to dev.\n");
		return -1;
	}

	dev->motor_fd = open(MOTOR_PATH, O_RDWR);
	if(dev->motor_fd < 0) {
		syslog(LOG_ERR, "EE242_PRJ2:motor_dev_open: Failed to open motor_ctrl.\n");
		return  -1;
	}

	return 0;
}

void motor_dev_close(motor_dev *dev) {
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:motor_dev_close: Null pointer to dev.\n");
		return;
	}

	motor_cfg m_cfg;
	m_cfg.period_ns = 0;
	m_cfg.duty_ns = 0;
	motor_set_cfg(dev, m_cfg);
	motor_disable(dev);
	close(dev->motor_fd);
}
