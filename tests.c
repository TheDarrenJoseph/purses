#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>

#define EPS 0.01

void assert_complex(complex_t expected, complex_t actual) {	
	
	double expected_real = expected.real;
	double expected_imag = expected.imaginary;
	double actual_real = actual.real;
	double actual_imag = actual.imaginary;
		
	// Use absolute to get positive numbers
	double real_diff = (fabs(expected_real - actual_real));
	double imag_diff = (fabs(expected_imag - actual_imag));
		
	// Check the diffs are below our Epsilon threshold
	if (!((real_diff < EPS) && (imag_diff < EPS))) {
		printf("Assertion failed!\nExpected Real: %.2f Imag: %.2f\nActual Real: %.2f, Imag: %.2f\n", 
		expected.real, expected.imaginary, actual.real, actual.imaginary);
		
		printf("Real diff: %.2f, Imag diff: %.2f, Epsilon: %.4f\n", real_diff, imag_diff, EPS);
		
		exit(1);
	}	
}


void print_data(complex_n_t* samples, int sample_rate) {
	int data_size = samples -> data_size;
	int nyquist = data_size / 2;
	// Frequency resolution = Sampling Freq / Sample Count
	int freq_resolution = sample_rate / data_size;	
	printf("Frequency Resolution: %dHz\n", freq_resolution);
	for (int i=0; i<nyquist; i++) {
		int frequency = freq_resolution * i;
		complex_t sample = samples -> data[i];
		double realval = sample.real;
		double imval = sample.imaginary;
		double mag = sample.magnitude;
		printf("(%d - %dHz) - Real: %.2f, Imaginary: %.2f\n", i, frequency, realval, imval);
		printf("(%d - %dHz) Magnitude: %.2f\n", i, frequency, mag);
	}
}

// Simple unit test functions to check we're on-point with our algos
// Credits to Simon Xu for the example
void test_dft() {
	
	int sample_rate = 8;
	complex_t empty = { 0.0 , 0.0 };

	// Test scenario, 1 second of recording, 8 samples (bytes) at 8Hz
	// of sine wave 1Hz, so we have 8 samples our our 1Hz wave
	// Allocate stuct memory space

	complex_n_t* complex_samples = (complex_n_t*) malloc(sizeof(complex_n_t));
	complex_samples -> data_size = 8;
	complex_t* data = complex_samples -> data;
	data = (complex_t*) malloc(sizeof(complex_t) * 8);
	
	complex_t sample0 = { 0.0 , 0.0 };
	complex_t sample1 = { 0.707 , 0.0 };
	complex_t sample2 = { 1.0 , 0.0 };
	complex_t sample3 = { 0.707 , 0.0 };
	complex_t sample4 = { 0.0 , 0.0 }; // And inverse
	complex_t sample5 = { -0.707, 0.0 };
	complex_t sample6 = { -1.0, 0.0 };
	complex_t sample7 = { -0.707 , 0.0 };
	
	
	data[0] = sample0;
	data[1] = sample1;
	data[2] = sample2;
	data[3] = sample3;
	data[4] = sample4;
	data[5] = sample5;
	data[6] = sample6;
	data[7] = sample7;
	complex_samples -> data = data;
	
	printf("=== Input Data ===\n");
	print_data(complex_samples, sample_rate);
	
	complex_n_t* output = (complex_n_t*) malloc(sizeof(complex_n_t));
	output -> data_size = 8;
	output -> data = (complex_t*) malloc(sizeof(complex_t) * 8);
	for (int i=0; i<8; i++) {
		output -> data[i] = empty;
	}
	
	printf("=== Result Data (Empty) ===\n");
	print_data(output, sample_rate);
	
	dft(complex_samples, output);
	set_magnitude(output, sample_rate, 8);
	
	// Nyquist limit == sample_rate/2
	int nyquist_lim = sample_rate/2;

	
	printf("=== Result Data ===\n");
	print_data(output, sample_rate);
	// Reduce the data size accordingly
	output -> data_size = 4;

	
	// Any complex number whos imaginary part is 0 can be treated as a real
	complex_t* output_data = output -> data;
	assert_complex((complex_t) {0.00, 0.00}, output_data[0]);
	assert_complex((complex_t) {0.00, -4.00}, output_data[1]);
	assert_complex((complex_t) {0.00, 0.00}, output_data[2]);
	assert_complex((complex_t) {0.00, 0.00}, output_data[3]);
	assert_complex((complex_t) {0.00, 0.00}, output_data[4]);
	assert_complex((complex_t) {0.00, 0.00}, output_data[5]);
	assert_complex((complex_t) {0.00, 0.00}, output_data[6]);
	assert_complex((complex_t) {0.00, 4.00}, output_data[7]);
	
	/**
	assert(output_data[1].real == 0.00);
	assert(output_data[1].imaginary == -4.00);
	assert(output_data[2].real == 0.00);
	assert(output_data[2].imaginary == 0.00);
	assert(output_data[3].real == 0.00);
	assert(output_data[3].imaginary == 0.00);
	assert(output_data[4].real == 0.00);
	assert(output_data[4].imaginary == 0.00);
	assert(output_data[5].real == 0.00);
	assert(output_data[5].imaginary == 0.00);
	assert(output_data[6].real == 0.00);
	assert(output_data[6].imaginary == 0.00);
	assert(output_data[7].real == 0.00);
	assert(output_data[7].imaginary == 4.00);
	*/
	
	
}


int main(void) {
	test_dft();
	printf("=== Tests Complete ===\n");
}
