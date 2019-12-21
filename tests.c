#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>


void print_data(complex_n_t* samples) {
	int data_size = samples -> data_size;
	for (int i=0; i<data_size; i++) {
		complex_t sample = samples -> data[i];
		double realval = sample.real;
		double imval = sample.imaginary;
		printf("(%d) - Real: %f, Imaginary: %f\n", i, realval, imval);
	}
}

// Simple unit test functions to check we're on-point with our algos
// Credits to Simon Xu for the example
void test_dft() {

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
	
	print_data(complex_samples);
	
}


int main(void) {
	test_dft();
}
