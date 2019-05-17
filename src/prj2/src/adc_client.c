#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>

#include "adc_client.h"

#include <stdio.h>


int adc_read(adc_dev adc, adc_raw *buff)
{
	char raw_buffer[6] = {0,0,0,0,0,0};
	ssize_t bytes_read;

	if(adc.adc_fd < 0) {
		syslog(LOG_ERR, "EE242_PRJ1:adc_dev_read: Failed to open ADC file.\n");
		return  -1;
	}
	
	lseek(adc.adc_fd, 0, SEEK_SET);
	bytes_read = read(adc.adc_fd, &raw_buffer, 4);
	*buff = atoi(raw_buffer);
	
	return 0;
}

int adc_dev_open(adc_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:adc_dev_open: NULL pointer to adc_dev.\n");
		return  -1;
	}

	dev->adc_fd = open(ADC_PATH, O_RDONLY);
	if(dev->adc_fd < 0) {
		syslog(LOG_ERR, "EE242_PRJ1:adc_dev_open: Failed to open ADC.\n");
		return  -1;
	}

	return 0;
}

void adc_dev_close(adc_dev *dev)
{
	if(dev == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:adc_dev_close: NULL pointer to adc_dev.\n");
		return;
	}

	close(dev->adc_fd);
}
