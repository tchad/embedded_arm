#ifndef _ADC_CLIENT_H_
#define _ADC_CLIENT_H_

#include <stdint.h>

#define ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage3_raw"

#define ADC_MIN 0
#define ADC_MAX 4095
#define HALF_RNG ((ADC_MAX-ADC_MIN)/2.0f)

typedef uint32_t adc_raw;

typedef struct _adc_dev {
	int adc_fd;
	
} adc_dev;

int adc_read(adc_dev adc, adc_raw *buff);
int adc_dev_open(adc_dev *dev);
void adc_dev_close(adc_dev *dev);

static inline float adc_normalize(float v)
{
	return (v-(ADC_MIN+HALF_RNG))/HALF_RNG;
}

#endif
