#include "adc_sampler.h"
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <math.h>
#include <unistd.h>
 
/*

s_sample* s_sample_adc(long sample_time_s, long ival_us, adc_dev *adc, int *sz)
{

	struct timeval sampling_begin;
	struct timeval sampling_now;

	struct timeval start;
	struct timeval finish;

	struct timespec init_req;
	struct timespec req;
	struct timespec remain;

	int sleep_ret;
	int data_idx;
	int i;
	
	*sz = (sample_time_s * 1e6)/ival_us + 500;
	s_sample *buff = (s_sample*)malloc((*sz)*sizeof(s_sample));
	for(i=0; i< *sz; ++i){
		buff[i].valid = 0;
	}

	data_idx = 0;
	init_req.tv_sec = 0;
	init_req.tv_nsec = ival_us * 1000;

	//Countdown 5 seconds
	i = 5;
	while(i) {
		printf("Sampling begin in %d\n", i);
		--i;
		sleep(1);
	}

	gettimeofday(&sampling_begin, NULL);
	sampling_now = sampling_begin;
	start = sampling_begin;

	printf("START!!!\n");
	while((sampling_now.tv_sec - sampling_begin.tv_sec) < sample_time_s) {
		adc_read(*adc, &(buff[data_idx].data_raw));
		gettimeofday(&finish, NULL);
		buff[data_idx].valid = 1;
		buff[data_idx].time_us = data_idx * ival_us;
		buff[data_idx].ival_us = finish.tv_usec - start.tv_usec;

		++data_idx;
		req = init_req;
		gettimeofday(&start, NULL);

		while(1) {
			sleep_ret = nanosleep(&req, &remain);

			if(sleep_ret == 0) {
				break;
			}

			req = remain;
		}

		gettimeofday(&sampling_now, NULL);
	}

	printf("DONE!!!\n");

	return buff;
}

int s_save_to_file(const char* filename, s_sample* data, int sz)
{
	if(data == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:s_save_to_file: NULL pointer to data.\n");
		return  -1;
	}

	FILE *f = fopen(filename, "w");
	if(f == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:s_sample: Failed to open %s to write.\n", filename);
		return  -1;
	}

	int i;
	for(i = 0; i< sz; ++i) {
		if(data[i].valid != 1){
			break;
		}
		s_sample d = data[i];
		fprintf(f, "%d:%d:%d\n", d.time_us, d.data_raw, d.ival_us);
	}

	fclose(f);
	return 0;

}

*/

inline static void s_sample_sleep(int sleep_ns)
{
    struct timespec req;
    struct timespec remain;
    int sleep_ret;
    req.tv_sec = 0;
    req.tv_nsec = sleep_ns;

    while(1) {
        sleep_ret = nanosleep(&req, &remain);

        if(sleep_ret == 0) {
            break;
        }

        req = remain;
    }
}

int s_sample_adc2(adc_dev *adc, int **b_data, int **b_dt, double *avg_dt, int *buff_sz, int pw, int sleep_ns)
{

    int sz;
	unsigned long i;

    sz = pow(2,pw);
	int *buff = (int*)malloc((sz)*sizeof(int));
	int *dt = (int*)malloc((sz)*sizeof(int));
    double avg;


	//Countdown 5 seconds
	i = 5;
	while(i) {
		printf("Sampling begin in %d\n", i);
		--i;
		sleep(1);
	}

    struct timeval start;
    struct timeval last;
    struct timeval current;
    struct timeval end;

    gettimeofday(&start, NULL);
    last = start;

	printf("START!!!\n");
	for(i=0; i< sz; ++i){
        gettimeofday(&current, NULL);
        dt[i] = current.tv_sec*1e6 + current.tv_usec - (last.tv_sec*1e6 + last.tv_usec);
        last = current;

		adc_read(*adc, &(buff[i]));

		if(sleep_ns != 0) {
            s_sample_sleep(sleep_ns);
		}
	}
	printf("DONE!!!\n");

    avg = 0;
	for(i=0; i< sz; ++i){
        avg += dt[i];
    }
    avg = avg/sz;

    gettimeofday(&end, NULL);
    int total = end.tv_sec*1e6 + end.tv_usec - (start.tv_sec*1e6 + start.tv_usec);

    printf("Samples: %d\n",sz);
    printf("Average sample time [us]: %f\n", avg);
    printf("Toral sampling time [us]: %d\n", total);

    *b_data = buff;
    *b_dt = dt;
    *avg_dt = avg;
    *buff_sz = sz;

	return 0;
}

int s_save_to_file2(const char* filename, int* data, int sz)
{
	if(data == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:s_save_to_file: NULL pointer to data.\n");
		return  -1;
	}

	FILE *f = fopen(filename, "w");
	if(f == NULL) {
		syslog(LOG_ERR, "EE242_PRJ1:s_sample: Failed to open %s to write.\n", filename);
		return  -1;
	}

	int i;
	for(i = 0; i< sz; ++i) {
		fprintf(f, "%d\n", data[i]);
	}

	fclose(f);
	return 0;

}

