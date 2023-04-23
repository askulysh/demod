#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sched.h>
#include <getopt.h>
#include "liquid.h"

#define INPUT_BUF_SIZE 8192

typedef struct {
	int arg_samplerate;
	int samplerate;

	int arg_resamplerate;
	int resamplerate;

	int arg_bandwidth;
	int bandwidth;

	int arg_fm_deviation;
	int fm_deviation;

} args_t;

void print_help() {
	fprintf(stderr, "demod takes signed 16 bit IQ data stream filter_attenuation input and produces signed 16 bit demodulated audio filter_attenuation output\n");
	fprintf(stderr, "usage: demod args\n");
	fprintf(stderr, "\t--samplerate \t-s <samplerate>\t\t: input data stream samplerate\n");
	fprintf(stderr, "\t--resamplerate \t-r <resamplerate>\t: output data stream samplerate\n");
	fprintf(stderr, "\t--bandwidth \t-b <bandwidth_hz>\t: input signal bandwidth\n\n");
	fprintf(stderr, "\t--help \t\t-h \t\t\t: prints this usage information\n");
}

int main(int argc, char*argv[])
{
	int opt = 0;
	int long_index = 0;
	char* subopts;
	char* value;

	args_t args;
	memset((void*)&args, 0, sizeof(args_t));
	static struct option long_options[] = {
		{"samplerate",		required_argument,	0,	's' },
		{"resamplerate",	required_argument,	0,	'r' },
		{"bandwidth",		required_argument,	0,	'b' },
		{"help",		required_argument,	0,	'h' },
		{NULL,			0,			NULL,	 0  }
	};

	while ((opt = getopt_long(argc, argv,"s:r:b:m:h", long_options,
					&long_index )) != -1) {
		switch (opt) {
			case 's' :
				args.arg_samplerate = 1;
				args.samplerate = atoi(optarg);
				if (args.samplerate <= 0) {
					fprintf(stderr, "samplerate must be > 0\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'r' :
				args.arg_resamplerate = 1;
				args.resamplerate = atoi(optarg);
				if (args.resamplerate <= 0) {
					fprintf(stderr, "resamplerate must be > 0\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'b' :
				args.arg_bandwidth = 1;
				args.bandwidth = atoi(optarg);
				if (args.bandwidth <= 0) {
					fprintf(stderr, "bandwidth must be > 0 Hz\n");
					exit(EXIT_FAILURE);
				}
				break;

			case 'h' :
				print_help();
				exit(EXIT_SUCCESS);
				break;
			default:
				print_help();
				exit(EXIT_FAILURE);
		}
	}

	if (!args.arg_samplerate) {
		fprintf(stderr, "-s [--samplerate] not specified!\n");
		exit(EXIT_FAILURE);
	}
	if (!args.resamplerate) {
		args.resamplerate = args.samplerate;
	}

#if 0
	if (!args.bandwidth) {
		fprintf(stderr, "-b [--bandwidth] not specified!\n");
		exit(EXIT_FAILURE);
	}

	// filter options
	unsigned int filter_len = 64;
	float filter_cutoff_freq = (float)args.bandwidth / (float)args.samplerate;
	float filter_attenuation = 70.0f; // stop-band attenuation
	// design filter from prototype and scale to bandwidth
//	firfilt_crcf filter = firfilt_crcf_create_kaiser(filter_len, filter_cutoff_freq, filter_attenuation, 0.0f);
//	firfilt_crcf_set_scale(filter, 2.0f*filter_cutoff_freq);
#endif

	int M = args.samplerate/args.resamplerate;
	int M_rat = (float)args.samplerate/args.resamplerate;

	unsigned int m           = 8;       // filter delay
	float        As          = 60.0f;   // filter stop-band attenuation
	firdecim_crcf decim;

	float complex *c_input = malloc(INPUT_BUF_SIZE*M*2*sizeof(float));
	float complex decim_out[INPUT_BUF_SIZE];
	// FM demodulator
	//float kf = (float)args.fm_deviation/(float)args.resamplerate;
	freqdem fm_demodulator = freqdem_create(/*kf*/0.5);
	float complex *dem_in;
	float dem_out[INPUT_BUF_SIZE];

	if (args.samplerate % args.resamplerate == 0)
		M_rat = 0;
	else
		M = 0;

        if (M > 1) {
		decim = firdecim_crcf_create_kaiser(M, m, As);
		firdecim_crcf_set_scale(decim, 1.0f/(float)M);
		dem_in = decim_out;
	} else {
		dem_in = c_input;
	}

	while (1) {
		int items_read = fread(c_input, M*2*sizeof(float),
				INPUT_BUF_SIZE, stdin);
		if (items_read) {
			if (M != 1) {
				firdecim_crcf_execute_block(decim, c_input,
						items_read, decim_out);
			}
			freqdem_demodulate_block(fm_demodulator, dem_in,
					items_read, dem_out);

			if (0) {
				//float output
				fwrite(dem_out, sizeof(float),
						items_read, stdout);
			} else {
				for (int i=0; i<items_read; i++) {
#if 0
					if (dem_out[i] > 1.0) {
						printf("%f ", dem_out[0]);
						dem_out[i] = 1.0;
					}
					if (dem_out[i] < -1.0) {
						printf("%f ", dem_out[0]);
						dem_out[i] = -1.0;
					}
#endif
					((int16_t*)&dem_out)[i] = dem_out[i] * 32767.0;

				}

				// i16 output
				fwrite(dem_out, sizeof(uint16_t),
						items_read, stdout);
			}

			fflush(stdout);
			sched_yield();
		}

		if (feof(stdin)) {
			break;
		}
	}

	firdecim_crcf_destroy(decim);

	// destroy fm demodulator object
	freqdem_destroy(fm_demodulator);

	return 0;
}
