
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>

#include "compensator.h"

int comp_init_model(const char *filename, comp_model *model)
{
	uint32_t i = 0;
	if(model == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:comp_init_model: NULL pointer to model.\n");
		return  EXIT_FAILURE;
	}
	
	FILE *comp_fd = fopen(filename, "r");
	if(comp_fd == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:comp_init_model: Failed to open %s.\n", filename);
		return  EXIT_FAILURE;
	}

	fscanf(comp_fd, "%u", &(model->n));

	model->lut = (comp_point*)malloc(sizeof(comp_point)*model->n);

	for(i = 0; i<model->n; ++i) {
		comp_point p;
		fscanf(comp_fd, "%f %f %f %f %u %u", &(p.p), &(p.q), &(p.ag), &(p.bg),
			 	&(p.range_begin), &(p.range_end));
		model->lut[i] = p;
	}

	fclose(comp_fd);
	return EXIT_SUCCESS;

}

int comp_destroy_model(comp_model *model)
{
	if(model == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:comp_destroy_model: NULL pointer to model.\n");
		return  EXIT_FAILURE;
	}

	free(model->lut);

	return EXIT_SUCCESS;
}

float comp_compensate(const comp_model* model, adc_raw f)
{
	//Find the corresponding range
	uint32_t idx_range = comp_search_by_raw_out(model, f);
	comp_point r = model->lut[idx_range];

	//Use p and q to get original input
	float v_in = (f-r.q)/r.p;

	//compute g(x)
	float g = v_in*r.ag + r.bg;

	//Rerurn compensated f+g
	return f+g;
}

uint32_t comp_search_by_raw_out(const comp_model *model, adc_raw val)
{
	/*
	 * Assuming correct input data
	 * Binary search by raw ADC data
	 */
	uint32_t begin = 0;
	uint32_t end = (model->n) - 1;

	while( end != begin) {
		uint32_t m = comp_mid(begin, end);
		int pos = comp_check_range(&(model->lut[m]),val);

		if(pos == 0) {
			return m;
		}
		else if(pos == -1) {
			end = m-1;
		}
		else { // pos == 1
			begin = m + 1;
		}
	}

	return begin;
}

