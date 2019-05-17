#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>

#include "service.h"

volatile int daemon_run = 1;
volatile int mode_adc_sample = 0;
volatile int mode_compass = 0;
volatile int sample_delay = 0;
volatile int samples_power = 17;

void print_usage(FILE *s)
{
    fprintf(s,
        "Usage: \n"
        "   -a  		--application   Start in application mode.\n"
        "   -d  		--daemon        Start in daemon mode.\n"
	"   -s  		--sample    	Sample ADC data.\n"
	"   -c			--compass       Print compass data.\n"
	"   -r <int>		--delay	    	Sampling delay in ns(default 0).\n"
	"   -p <int>		--power	    	samples_taken = 2^p\n"
        "   -h			--help	    	Print this message.\n");
}

//Signal handler closing the service
void exit_sig_handler(int sig)
{
    daemon_run = 0;
    syslog(LOG_NOTICE, "EE242_PRJ2: caught terminating signal, shutting down.\n");
}


int run_daemon() {
    //Daemonize the process
    pid_t pid = fork();

    if(pid < 0) {
        printf("EE242_PRJ1: Unable to fork into daemon");
        exit(EXIT_FAILURE);
    }

    if(pid > 0) {
        exit(EXIT_SUCCESS);
    } else {
        umask(0);
	openlog("EE242_PRJ1", LOG_PID|LOG_NOWAIT,LOG_USER);
        syslog(LOG_NOTICE, "EE242_PRJ1: Starting daemon.\n");

	pid_t sid = setsid();
        if(sid < 0) {
            syslog(LOG_ERR, "EE242_PRJ1: Error creating process group.\n");
            exit(EXIT_FAILURE);
        }
        
        if(chdir("/") < 0) {
            syslog(LOG_ERR, "EE242_PRJ1: Error changing working directory to /.\n");
            exit(EXIT_FAILURE);
        }

	close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

	//Set signal handlers
	if(signal(SIGINT, exit_sig_handler) == SIG_ERR){
	    syslog(LOG_ERR, "EE242_PRJ1: unable to assign signal handler");
	    exit(EXIT_FAILURE);
	}

	if(signal(SIGTERM, exit_sig_handler) == SIG_ERR){
	    syslog(LOG_ERR, "EE242_PRJ1: unable to assign signal handler");
	    exit(EXIT_FAILURE);
	}

	if(signal(SIGHUP, exit_sig_handler) == SIG_ERR){
	    syslog(LOG_ERR, "EE242_PRJ1: unable to assign signal handler");
	    exit(EXIT_FAILURE);
	}

	if(signal(SIGQUIT, exit_sig_handler) == SIG_ERR){
	    syslog(LOG_ERR, "EE242_PRJ1: unable to assign signal handler");
	    exit(EXIT_FAILURE);
	}

        int ret = run();

	closelog();

	return ret;
    }

}

int run_application()
{
        openlog("EE242_PRJ1", LOG_PID|LOG_NOWAIT,LOG_USER);
        syslog(LOG_NOTICE, "EE242_PRJ1: Starting local.\n");

        int ret = run();

        closelog();

	return ret;
}

int main(int argc, char *argv[])
{
    const char* const short_options = "adchsr:p:";

    const struct option long_options[] = {
        { "help",   no_argument,  NULL,  'h'},
        { "daemon", no_argument,  NULL,  'd'},
        { "sample", no_argument,  NULL,  's'},
        { "compass", no_argument,  NULL,  'c'},
	{ "delay", required_argument, NULL, 'r'},
	{ "power", required_argument, NULL, 'p'},
        { "application", 0,  NULL,  'a'},
        { NULL,     0,  NULL,   0 }
    };

    int option;
    int is_daemon = 0;


    do{
        option = getopt_long(argc, argv, short_options, long_options, NULL);
        
        switch(option) {
            case 'h':
                print_usage(stdout);
                exit(EXIT_SUCCESS);
                break;
            case 'd':
                is_daemon = 1;
                break;
	    case 'a':
		    is_daemon = 0;
		    break;
	    case 'c':
		    mode_compass = 1;
		    break;
	    case 's':
		    mode_adc_sample = 1;
		    break;
	    case 'r':
		    sample_delay = atoi(optarg);
		    break;
	    case 'p':
		    samples_power = atoi(optarg);
            break;
        case -1:
            break;
        default:
            print_usage(stdout);
            abort();
        };
    }
    while (option != -1);

    int ret;

    if(is_daemon) {
        ret =  run_daemon();
    }
    else {
        ret = run_application();
    }

    return ret;
}
