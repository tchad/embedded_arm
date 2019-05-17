#include "adc_client.h"
#include <stdint.h>

#ifndef __ADC_SAMPLER_H__
#define __ADC_SAMPLER_H__

#define S_OUT_PATH "/adc_sampler.out"
#define S_DT_PATH "/adc_sampler_dt.out"

int s_sample_adc2(adc_dev *adc, int **b_data, int **b_dt, double *avg_dt, int *buff_sz, int pw, int sleep_ns);
int s_save_to_file2(const char* filename, int* data, int sz);

#endif

