#ifndef __MAG_CLIENT_H__
#define __MAG_CLIENT_H__

#include <stdint.h>
#include <pthread.h>

#define MAG_PATH "/dev/lsm303_mag"

typedef struct _mag_data {
	int16_t x;
	int16_t y;
	int16_t z;
} mag_data;

typedef struct _mag_dev {
	int mag_fd;
	mag_data buff;
	int new_data;

	int run;
	pthread_t thread;
	pthread_mutex_t mutex;
} mag_dev;

void mag_start_read_thread(mag_dev *dev);
void mag_stop_read_thread(mag_dev *dev);

/*
 * ret:
 * -1 - error reading
 *  0 - no new data
 *  1 -success, new data
*/
int mag_read_data(mag_dev *dev, mag_data *data);
float mag_heading(mag_data data);

int mag_dev_open(mag_dev *dev);
void mag_dev_close(mag_dev *dev);

#endif
