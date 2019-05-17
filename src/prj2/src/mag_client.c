
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include "mag_client.h"

static void* mag_read_thread_fn(void *arg) {
	mag_dev *dev = (mag_dev*) arg;

	while(dev->run) {
		mag_data tmp;
		read(dev->mag_fd, &tmp, sizeof(tmp));

		pthread_mutex_lock(&dev->mutex);
		dev->buff = tmp;
		dev->new_data = 1;
		pthread_mutex_unlock(&dev->mutex);
	}

	pthread_exit(NULL);
}

void mag_start_read_thread(mag_dev *dev)
{
	pthread_mutexattr_t attr;
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_start_read_thread: Null pointer to dev.\n");
		return;
	}

	if(dev->run) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_start_read_thread: thread already running.\n");
		return;
	}

	dev->run = 1;
	dev->new_data = 0;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&dev->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	pthread_create(&dev->thread, NULL, mag_read_thread_fn, dev);
}

void mag_stop_read_thread(mag_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_stop_read_thread: Null pointer to dev.\n");
		return;
	}

	if(dev->run) {
		dev->run = 0;
		pthread_join(dev->thread, NULL);
		pthread_mutex_destroy(&dev->mutex);
	}
}

int mag_read_data(mag_dev *dev, mag_data *data)
{
	int ret = 0;

	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_read_data: Null pointer to dev.\n");
		return -1;
	}

	pthread_mutex_lock(&dev->mutex);
	if(dev->new_data) {
		*data = dev->buff;
		ret = 1;
	} else {
		ret = 0;
	}
	pthread_mutex_unlock(&dev->mutex);

	return ret;
}

float mag_heading(mag_data data)
{
	float tmp = atan2f(data.y, data.x);
	tmp = tmp * 180.0/M_PI;

	if(tmp < 0) {
		tmp = 360 + tmp;
	}

	return tmp;
}

int mag_dev_open(mag_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_dev_open: Null pointer to dev.\n");
		return -1;
	}

	dev->mag_fd = open(MAG_PATH, O_RDONLY);

	if(dev->mag_fd < 0) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_dev_open: Failed to open lsm303_magmeter.\n");
		return  -1;
	}

	dev->run = 0;

	return 0;
}

void mag_dev_close(mag_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ2:mag_dev_close: Null pointer to dev.\n");
		return;
	}

	if(dev->run) {
		mag_stop_read_thread(dev);
	}

	close(dev->mag_fd);
}
