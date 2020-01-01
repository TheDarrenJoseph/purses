#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>

#define EPS 0.01

void assert_complex(double complex expected, double complex actual) {	
	
	double expected_real = creal(expected);
	double expected_imag = cimag(expected);
	double actual_real = creal(actual);
	double actual_imag = cimag(actual);
		
	// Use absolute to get positive numbers
	double diff = (fabs(expected - actual));
		
	// Check the diffs are below our Epsilon threshold
	if (!((diff < EPS))) {
		printlncol(ANSI_RED, "=== Assertion failed! ===");
		printf("Expected Real: %.2f Imag: %.2f\nActual Real: %.2f, Imag: %.2f\n", 
		expected_real, expected_imag, actual_real, actual_imag);
		
		printf("diff: %.2f, Epsilon: %.4f\n", diff, EPS);
		
		exit(1);
	} else {
			printf("Asserting:\nExpected Real: %.2f Imag: %.2f\nActual Real: %.2f, Imag: %.2f\n", 
		expected_real, expected_imag, actual_real, actual_imag);
	}
}


void print_data(complex_set_t* samples, int sample_rate) {
	int data_size = samples -> data_size;
	// Frequency resolution = Sampling Freq / Sample Count
	int freq_resolution = sample_rate / data_size;	
	printf("Frequency Resolution: %dHz\n", freq_resolution);
	
	double sum = 0.0;
	for (int i=0; i<data_size; i++) {
		int frequency = freq_resolution * i;
		struct complex_wrapper wrapper = samples -> complex_numbers[i];
		
		complex double complex_val = wrapper.complex_number;
		
		double realval = creal(complex_val);
		sum += realval;
		double imval = cimag(complex_val);
		double mag = wrapper.magnitude;
		printf("(%d - %dHz) - Real: %.2f, Imaginary: %+.2fi, Magnitude: %.2f\n", i, frequency, realval, imval, mag);
	}
	
}

// Simple unit test functions to check we're on-point with our algos
// Credits to Simon Xu for the example
void test_dft_1hz_8hz() {
	
	int sample_rate = 8;
	double complex empty = CMPLX(0.00, 0.0);

	// Test scenario, 1 second of recording, 8 samples (bytes) at 8Hz
	// of sine wave 1Hz, so we have 8 samples our our 1Hz wave
	// Allocate stuct memory space

	complex_set_t* complex_samples = (complex_set_t*) malloc(sizeof(complex_set_t));
	complex_samples -> data_size = 8;
	complex_wrapper_t* data = complex_samples -> complex_numbers;
	data = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * 8);
	
	double complex sample0 = CMPLX(0.0 , 0.0);
	double complex sample1 = CMPLX(0.707 , 0.0);
	double complex sample2 = CMPLX(1.0 , 0.0);
	double complex sample3 = CMPLX(0.707 , 0.0);
	double complex sample4 = CMPLX(0.0 , 0.0); // And inverse
	double complex sample5 = CMPLX(-0.707, 0.0);
	double complex sample6 = CMPLX(-1.0, 0.0);
	double complex sample7 = CMPLX(-0.707 , 0.0);
	
	
	data[0].complex_number = sample0;
	data[1].complex_number = sample1;
	data[2].complex_number = sample2;
	data[3].complex_number = sample3;
	data[4].complex_number = sample4;
	data[5].complex_number = sample5;
	data[6].complex_number = sample6;
	data[7].complex_number = sample7;
	complex_samples -> complex_numbers = data;
	
	printf("=== Input Data ===\n");
	print_data(complex_samples, sample_rate);
	
	complex_set_t* output = (complex_set_t*) malloc(sizeof(complex_set_t));
	output -> data_size = 8;
	output -> complex_numbers = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * 8);
	for (int i=0; i<8; i++) {
		output -> complex_numbers[i] = (complex_wrapper_t) { empty, 0.0 };
	}
	
	printf("=== Result Data (Empty) ===\n");
	print_data(output, sample_rate);
	
	dft(complex_samples, output);
	nyquist_filter(output, 8);
	set_magnitude(output, 8);
	
	// Nyquist limit == sample_rate/2
	int nyquist_lim = sample_rate/2;

	
	printf("=== Result Data ===\n");
	print_data(output, sample_rate);
	// Reduce the data size accordingly
	output -> data_size = 4;

	
	// Any complex number whos imaginary part is 0 can be treated as a real
	complex_wrapper_t* output_data = output -> complex_numbers;
	assert_complex(CMPLX(0.00, 0.00), output_data[0].complex_number);
	assert_complex(CMPLX(0.00, -4.00), output_data[1].complex_number);
	assert_complex(CMPLX(0.00, 0.00), output_data[2].complex_number);
	assert_complex(CMPLX(0.00, 0.00), output_data[3].complex_number);
	assert_complex(CMPLX(0.00, 0.00), output_data[4].complex_number);
	assert_complex(CMPLX(0.00, 0.00), output_data[5].complex_number);
	assert_complex(CMPLX(0.00, 0.00), output_data[6].complex_number);
	assert_complex(CMPLX(0.00, 4.00), output_data[7].complex_number);
}

// Wikipedia DFT Example
void test_dft_wiki_example() {
	
	int data_size = 4;
	int sample_rate = 4;
	double complex empty = CMPLX(0.00, 0.0);
	
	complex_set_t* complex_samples = (complex_set_t*) malloc(sizeof(complex_set_t));
	complex_samples -> data_size = data_size;
	complex_wrapper_t* data = complex_samples -> complex_numbers;
	data = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * 8);
	
	double complex sample0 = CMPLX(1.0 , 0.0);
	double complex sample1 = CMPLX(2.0 , cimag(-(1*I)));
	double complex sample2 = CMPLX(0.0 , cimag(-(1*I)));
	double complex sample3 = CMPLX(-1.0, cimag(2*I));
	
	printf("Imaginary numbers, 1: %.2f, 2: %.2f, 3: %.2f, 4: %.2f\n", cimag(sample0), cimag(sample1), cimag(sample2), cimag(2*I));
	
	data[0].complex_number = sample0;
	data[1].complex_number = sample1;
	data[2].complex_number = sample2;
	data[3].complex_number = sample3;
	complex_samples -> complex_numbers = data;
	
	printf("=== Input Data ===\n");
	print_data(complex_samples, sample_rate);
	
	complex_set_t* output = (complex_set_t*) malloc(sizeof(complex_set_t));
	output -> data_size = data_size;
	output -> complex_numbers = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * 8);
	for (int i=0; i<8; i++) {
		output -> complex_numbers[i] = (complex_wrapper_t) { empty, 0.0 };
	}
	
	printf("=== Result Data (Empty) ===\n");
	print_data(output, sample_rate);
	
	dft(complex_samples, output);
	// Print and assert without any post-processing
	printf("=== Result Data ===\n");
	print_data(output, sample_rate);
	// Reduce the data size accordingly
	output -> data_size = data_size;
	
	// Any complex number whos imaginary part is 0 can be treated as a real
	complex_wrapper_t* output_data = output -> complex_numbers;
	assert_complex(CMPLX(2.00, 0.00), output_data[0].complex_number);
	assert_complex(CMPLX(-2.00, -2.00*I), output_data[1].complex_number);
	assert_complex(CMPLX(0.00, -2.00*I), output_data[2].complex_number);
	assert_complex(CMPLX(4.00, 4.00*I), output_data[3].complex_number);
}

/**
 * For generating test data
 **/
void sine(record_stream_data_t* record_data, int frequency, int sample_rate, int sample_count) {
	// Up to 360 degrees
	double max_radians = 360*M_PI/180;
	
	int max_amplitude = INT16_MAX;
	
	int increment = max_radians / sample_rate / frequency;
	printf("Max amplitude: %d, Sample Rate: %dHz, Sine Frequency: %dHz, Increment: %dHz\n", max_amplitude, sample_rate, frequency, increment);

	for (int rads=0; rads<max_radians; rads += increment) {
		int16_t sample = max_amplitude * sin(rads);
		printf("Sample (%d radians) = %d\n", rads, sample);
	}
}

void generate_sine_10khz_44100hz() {
	record_stream_data_t* sample_date = 0;
	init_record_data(&sample_date);
	sine(sample_date, 10, 44100, 44100);
}

void test_dft_10khz_44100hz() {
		double complex empty = CMPLX(0.00, 0.0);
	
		// 1. Read input from white noise file
		// 44100 16 bit PCM samples @ 44100Hz
		int sample_rate = 44100;
		int sample_count = 44100; // TODO Make this 10x the sample rate for 10Hz increments, etc
		int16_t* recorded_samples = (int16_t*) malloc(sizeof(int16_t) * sample_count);
		record_stream_data_t* file_read_data = 0;
		init_record_data(&file_read_data);

		read_from_file(file_read_data, "record.bin");

		// 2. Convert these into complex_t types with 0 imaginary
		
		// Initialise the complex samples
		complex_set_t* complex_samples = (complex_set_t*) malloc(sizeof(complex_set_t));
		complex_samples -> data_size = sample_count;
		complex_samples -> complex_numbers = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * sample_count);
		complex_wrapper_t* data = complex_samples -> complex_numbers;
		
		for (int i=0; i<sample_count; i++) {
			int16_t sample = file_read_data -> data[i];
			printf("Read sample (%d) : %d\n", i, sample);
			data[i].complex_number = CMPLX((double) sample, 0.00);
			data[i].magnitude = 0.00;
		}
		
		printf("=== Input Data ===\n");
		print_data(complex_samples, sample_rate);

		
		// 3. Run through DFT
		complex_set_t* output = (complex_set_t*) malloc(sizeof(complex_set_t));
		output -> data_size = sample_count;
		output -> complex_numbers = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * sample_count);
		for (int i=0; i<sample_count; i++) {
			output -> complex_numbers[i].complex_number = empty;
			output -> complex_numbers[i].magnitude = 0.00;
		}
		dft(complex_samples, output);
		nyquist_filter(output, sample_count);
		set_magnitude(output, sample_count);

		
		// 4. Assert output magnitude agrees for all frequencies
		// Frequency resolution is 1Hz (512/512)
		// So we have output of 0-512Hz covered
		int freq_resolution = sample_rate / sample_count;	
		printf("Frequency Resolution: %dHz\n", freq_resolution);
		
		printf("=== Output Data ===\n");
		print_data(output, sample_rate);
		
}


int main(void) {
	test_dft_1hz_8hz();
	test_dft_wiki_example();
	//generate_sine_10khz_44100hz();
	//test_dft_10khz_44100hz();
	
	printlncol(ANSI_GREEN, "=== Tests Complete ===\n");
	
	/**
	printf("%s%s",ESC,ANSI_GREEN);
	printf("=== Tests Complete ===\n");
	printf("%s%s",ESC,ANSI_DEFAULT);
	**/

}
