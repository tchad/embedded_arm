#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
//#include <string.h>
#include <syslog.h>
#include <sys/stat.h>

#include "adc_client.h"
#include "compensator.h"
#include "motor_client.h"
#include "adc_sampler.h"
#include "gpio_client.h"
#include "mag_client.h"


/*
 * Unused, only for duty cycle speed driving
#define DUTY_CYCLE_MIN_PERCENT 5
#define DUTY_CYCLE_MAX_PERCENT 45
#define DUTY_CYCLE_RNG_PERCENT (DUTY_CYCLE_MAX_PERCENT - DUTY_CYCLE_MIN_PERCENT)
#define PERIOD_2kHz_ns 500000
*/

#define FREQ_MIN_HZ 2000
#define FREQ_MAX_HZ 20000
#define FREQ_RANGE (FREQ_MAX_HZ - FREQ_MIN_HZ)
#define CONV_S_TO_NS 10e9

#define ROTATION_TIME_MSEC 500

extern volatile int daemon_run;
extern volatile int mode_adc_sample;
extern volatile int sample_delay;
extern volatile int samples_power;
extern volatile int mode_compass;

static int run_mode_service(void)
{
        syslog(LOG_NOTICE, "EE242_PRJ2: Running service mode.\n");

	int err;

	adc_dev adc;
	comp_model cm;
	motor_dev md;
	gpio_dev gpio;
	mag_dev mag;

	btn_state btn_last;
	btn_state btn_curr;


	//Initialize subsystems
	err = adc_dev_open(&adc);
	if(err < 0) {
		return EXIT_FAILURE;
	}

	err = motor_dev_open(&md);
	if(err < 0) {
		adc_dev_close(&adc);
		return EXIT_FAILURE;
	}

	err = comp_init_model(COMP_PATH, &cm);
	if(err < 0) {
		adc_dev_close(&adc);
		motor_dev_close(&md);
		return EXIT_FAILURE;
	}

	err = gpio_dev_open(&gpio);
	if(err < 0) {
		adc_dev_close(&adc);
		motor_dev_close(&md);
		comp_destroy_model(&cm);
		return EXIT_FAILURE;
	}

	err = mag_dev_open(&mag);
	if(err < 0) {
		adc_dev_close(&adc);
		motor_dev_close(&md);
		comp_destroy_model(&cm);
		gpio_dev_close(&gpio);
		return EXIT_FAILURE;
	}
	mag_start_read_thread(&mag);


	//Default set
	motor_disable(&md);
	gpio_disable_led(&gpio);
	btn_last = BTN_RELEASED;
	btn_curr = BTN_RELEASED;

	while(daemon_run) {
		gpio_read_btn_state(&gpio, &btn_curr);
		if(btn_last == BTN_RELEASED && btn_curr == BTN_PRESSED) {

			mag_data mag_raw_begin;
			float heading_begin;
		        mag_data mag_raw_end;
			float heading_end;

			adc_raw adc_raw_val;
			float adc_comp_val;
			float adc_norm_val;

			float pwm_freq;
			motor_cfg m_cfg;


			while(mag_read_data(&mag, &mag_raw_begin) != 1){}

			err = adc_read(adc, &adc_raw_val);
			adc_comp_val = comp_compensate(&cm, adc_raw_val);
			adc_norm_val = adc_normalize(adc_comp_val);

			if(adc_norm_val < 0){
				adc_norm_val = -adc_norm_val;
				m_cfg.direction = 1; //CCW
			} else {
				m_cfg.direction = -1; //CW
			}

			/*
			 * Unused, only for speed driven by dutycycle
			float duty_cycle_percent;
			m_cfg.period_ns = PERIOD_2kHz_ns;
			duty_cycle_percent = DUTY_CYCLE_MIN_PERCENT + (adc_norm_val * DUTY_CYCLE_RNG_PERCENT);
			m_cfg.duty_ns = PERIOD_2kHz_ns * (duty_cycle_percent/100.0);
			*/

			pwm_freq = FREQ_MIN_HZ + (FREQ_RANGE * adc_norm_val);
			m_cfg.period_ns = (1.0/pwm_freq) * CONV_S_TO_NS;
			m_cfg.duty_ns = m_cfg.period_ns/2.0;


			gpio_enable_led(&gpio);

			motor_set_cfg(&md, m_cfg);
			motor_enable(&md);

			usleep(ROTATION_TIME_MSEC * 1000);

			motor_disable(&md);
			gpio_disable_led(&gpio);

			while(mag_read_data(&mag, &mag_raw_end) != 1){}

			heading_begin = mag_heading(mag_raw_begin);
			heading_end = mag_heading(mag_raw_end);

			printf("Begin: %f, End: %f, PWM freq %fHz\n", heading_begin, heading_end, pwm_freq);

		}
		btn_last = btn_curr;
	}


	//Shutting down
	motor_disable(&md);
	adc_dev_close(&adc);
	motor_dev_close(&md);
	comp_destroy_model(&cm);
	gpio_dev_close(&gpio);
	mag_dev_close(&mag);
	return EXIT_SUCCESS;
}

static int run_mode_compass(void)
{
        syslog(LOG_NOTICE, "EE242_PRJ2: Running compass mode.\n");

	int err;

	mag_dev mag;

	err = mag_dev_open(&mag);
	if(err < 0) {
		return EXIT_FAILURE;
	}
	mag_start_read_thread(&mag);

	while(daemon_run) {
		mag_data mag_raw_begin;
		float heading;

		while(mag_read_data(&mag, &mag_raw_begin) != 1){}
		heading = mag_heading(mag_raw_begin);
		printf("Compass X heading: %f\n", heading);

		usleep(1000000);
	}


	//Shutting down
	mag_dev_close(&mag);
	return EXIT_SUCCESS;
}

static int run_mode_adc_sampling(void)
{
	syslog(LOG_NOTICE, "EE242_PRJ1: Running sampling mode.\n");

	int err;
	adc_dev adc;

	err = adc_dev_open(&adc);
	if(err < 0) {
		return EXIT_FAILURE;
	}

	int buffer_size;
	int *data;
	int *dt;
	double avg_dt;

	s_sample_adc2(&adc, &data, &dt, &avg_dt, &buffer_size, samples_power, sample_delay);

	s_save_to_file2(S_OUT_PATH, data, buffer_size);
	s_save_to_file2(S_DT_PATH, dt, buffer_size);

	free(data);
	free(dt);
	adc_dev_close(&adc);

	return 0;

}

int run(void) {
	int ret; 
	
	if(mode_adc_sample == 1){
		ret = run_mode_adc_sampling();
	}
	else if(mode_compass == 1) {
		ret = run_mode_compass();
	}
	else {
		ret = run_mode_service();
	}

	return ret;
}

