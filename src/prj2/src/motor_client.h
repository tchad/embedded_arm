
#ifndef __MOTOR_CLIENT_H__
#define __MOTOR_CLIENT_H__

#define IOCTL_MOTOR_EN 0
#define IOCTL_MOTOR_CFG 1

#define MOTOR_PATH "/dev/motor_ctrl"

typedef struct _motor_cfg {
	int direction;
	int period_ns;
	int duty_ns;
} motor_cfg;

typedef struct _motor_state_desc {
	int enabled;
	motor_cfg cfg;
} motor_state_desc;

typedef struct _motor_dev {
	int motor_fd;
} motor_dev;

int motor_read(motor_dev *dev, motor_state_desc* buff);
int motor_enable(motor_dev *dev);
int motor_disable(motor_dev *dev);
int motor_set_velocity(motor_dev *dev, float v);
void motor_set_cfg(motor_dev *dev, motor_cfg cfg);

int motor_dev_open(motor_dev *dev);
void motor_dev_close(motor_dev *dev);


#endif
