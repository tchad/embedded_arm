#include <stdint.h>
#include <unistd.h>
#include "adc_client.h"

#ifndef __COMPENSATOR_H__
#define __COMPENSATOR_H__


#define COMP_PATH "/usr/share/ee242_prj2/comp.dat"

typedef struct _comp_point {
	float p;
	float q;
	float ag;
	float bg;
	uint32_t range_begin;
	uint32_t range_end;
} comp_point;

typedef struct _comp_model {
	uint32_t n;
	comp_point* lut;
} comp_model;

int comp_init_model(const char *filename, comp_model *model);
int comp_destroy_model(comp_model *model);
float comp_compensate(const comp_model* model, adc_raw f);
uint32_t comp_search_by_raw_out(const comp_model *model, adc_raw val);

static int inline comp_check_range(const comp_point* p, adc_raw val)
{
	/*
	 *  0 - in range
	 *  1 - above
	 * -1 - below
	 */

	if((val >= p->range_begin) && (val < p->range_end)) {
		return 0;
	}
	else if(val < p->range_begin) {
		return -1;
	}
	else { //val >= p->range_end
		return 1;
	}
}

static uint32_t inline comp_mid(uint32_t b, ssize_t e)
{
	float tmp = b+((e-b)/2.0f);

	if((tmp - (int)tmp) != 0) {
		return ((uint32_t)tmp)+1;
	}
	else {
		return (uint32_t)tmp;
	}
}


#endif
