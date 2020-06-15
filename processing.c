#include <processing.h>
#include <pulsehandler.h>
#include <shared.h>
#include <math.h>

/**
 * Due to the Nyquist limit (half of the sampling rate) 
 * We need to remove data samples above this frequency limit (zero them)
 * As well as double the remaining values to adjust for this
 **/ 
void nyquist_filter(complex_set_t* x, int sample_rate) {
	int nyquist_frequency = sample_rate / 2;
	int data_size = x -> data_size;
	int freq_resolution = sample_rate / data_size;		
	// Use the frequency resolution (step in Hz) to check against this limit
	int frequency = 0;
	for (int i=0; i<data_size; i++, frequency += freq_resolution){
		
		double complex* complex_sample = &(x -> complex_numbers[i].complex_number);
		double realval = creal((*complex_sample));
		double imval = cimag((*complex_sample));
		int non_zero = (realval != 0.0 || imval != 0.0);
		if (frequency < nyquist_frequency && non_zero) {
			printf("Adjusting: %.2f, %.2fi by x2 for Nyquist limit\n", realval, imval);
			(*complex_sample) *= 2;
		} else {
			(*complex_sample) = CMPLX(0.0, 0.0);
		}
	}
	
	// Reduce the data size available
	x -> data_size = nyquist_frequency;
}

void set_magnitude(complex_set_t* x, int sample_count) {
	int data_size = x -> data_size;
	for (int i=0; i<data_size; i++) {
		
		struct complex_wrapper* this_val = &(x -> complex_numbers[i]);
		double complex complex_val = this_val -> complex_number;
		
		// Calculate magnitude
		x -> complex_numbers[i].magnitude = ((cabs(complex_val)) / sample_count);
	}
}

// x - input set
// X - output set
void dft(complex_set_t* x, complex_set_t* X) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "Running DFT...\n");
	int N = x -> data_size;
	// Output index (k)
	for (int k=0; k < N; k++) {
		fprintf(logfile, "DFT output (%d)...\n", k);
		
		// Initialise output (X[k])
		double real_out = 0.0;
		double im_out = 0.0;
		
		// Summation over input index (x[n])
		for (int n=0; n < N; n++) {
			struct complex_wrapper* in_data = &(x -> complex_numbers[n]);
			double real_xn = creal(in_data -> complex_number);
			double im_xn =  cimag(in_data -> complex_number);
			
			// Multiply xn by our complex exponent
			//xn * e^-((i^2*M_PI*k*n)/N)
			
			// Now for Eueler's formula (for any real number x, given as radians)
			// e^ix = cos(x) + i*sin(x)
			// Where i is our imaginary number
			// x is the angle in radians
			double rads = (2*M_PI/N)*k*n;
			double real_inc = (real_xn * cos(rads)) + (im_xn * sin(rads));
			double imag_inc = (-real_xn * sin(rads)) + (im_xn * cos(rads));
			fprintf(logfile, "(Output %d/%d) Input Rads: %02f Real: %02f, Imaginary: %02f\n", k, n, rads, real_inc, imag_inc);
			real_out += real_inc;
			im_out += imag_inc;

		}	
		fprintf(logfile, "(Summation Output %d/%d) Real: %02f, Imaginary: %02f\n", k+1, N, real_out, im_out);
		double complex output = CMPLX(real_out, im_out);
		X -> complex_numbers[k].complex_number = output;
	}
}

void malloc_complex_set(complex_set_t** set, int sample_count) {
		// Initialise the complex samples
		(*set)  = (complex_set_t*) malloc(sizeof(complex_set_t));
		(*set)  -> data_size = sample_count;
		(*set)  -> complex_numbers = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * sample_count);
}

void build_complex_set(record_stream_data_t* record_data, complex_set_t** set, int sample_count) {
		FILE* logfile = get_logfile();
		malloc_complex_set(set, sample_count);

		complex_wrapper_t* data = (*set) -> complex_numbers;
		// Convert samples to Complex numbers
		for (int i=0; i<sample_count; i++) {
			int16_t sample = record_data -> data[i];
			fprintf(logfile, "Read sample (%d) : %d\n", i, sample);
			data[i].complex_number = CMPLX((double) sample, 0.00);
			data[i].magnitude = 0.00;
		}
}

/**
 * Initialises output_set from the record_stream
 */
void record_stream_to_complex_set(record_stream_data_t* record_stream, complex_set_t* output_set) {
	FILE* logfile = get_logfile();
	unsigned int size_n = record_stream -> data_size;
	
	int power_of_two = (size_n > 1 && (size_n%2) != 0);
	
	// Check for N power of 2
	if (size_n == 0 || power_of_two) {
		fprintf(logfile, "Cannot perform radix-2 processing if input data size if not a power of 2!, received data size: %d\n", size_n);
		return;
	}
	
	// Allocate input set
	complex_set_t* input_set = 0;
	build_complex_set(record_stream, &output_set, size_n);
}

// Output will be Audo Frequency (Hz/kHZ) mapped to a rough frequency scale (i.e 1..10); 
// Cooley-Turkey algorithm, radix 2
// This performs a radix-2 via a Decimation In Time (DIT) approach
void ct_fft(complex_set_t* input_data, complex_set_t* output_data) {
	FILE* logfile = get_logfile();
	unsigned int size_n = input_data -> data_size;

	// Trivial size-1 DFT 
	if (size_n == 1) {
		output_data[0] = input_data[0];
		return;
	}
	
	unsigned int half_size = size_n/2;
	
	fprintf(logfile, "=== Input Data ===\n");
	print_data(input_data, SAMPLE_RATE);
	
	fprintf(logfile, "FFT Processing %d bytes.\n", size_n);
	
	// 1. Separate input into an N/2 even and odd set
	complex_set_t* even_set = 0;
	complex_set_t* odd_set = 0;
	malloc_complex_set(&even_set, half_size);
	malloc_complex_set(&odd_set, half_size);
	complex_wrapper_t* data_n = input_data -> complex_numbers;
	complex_wrapper_t* data_even = even_set -> complex_numbers;
	complex_wrapper_t* data_odd = odd_set -> complex_numbers;
	for (int i=0; i < size_n; i++) {
		double complex complex_sample = data_n[i].complex_number;
		double realval = creal(complex_sample);
		if ((i%2) == 0) {
			fprintf(logfile, "(Even) Processing sample (%d): Real value: %.02f\n", i, realval);
			data_even[i] = data_n[i];
		} else {
			fprintf(logfile, "(Odd) Processing sample (%d): Real value: %.02f\n", i, realval);
			data_odd[i] = data_n[i];
		}
	}

	
	// 2. Perform DFT on each (2 DFTs of size half_size)
	complex_set_t* even_out_set = 0;
	complex_set_t* odd_out_set = 0;
	malloc_complex_set(&even_out_set, half_size);
	malloc_complex_set(&odd_out_set, half_size);
	dft (even_set, even_out_set);
	dft (odd_set, odd_out_set);
	
	
	complex_set_t* size2_set = 0;
	malloc_complex_set(&size2_set, 2);

	complex_set_t* size2_output_set = 0;
	malloc_complex_set(&size2_output_set, 2);
	
	//3. Combine results using butterfly approach 
	// Perform half_size DFTs of size 2
	for (int i=0; i < half_size; i++) {
		// Make pointers for our size-2 inputs
		complex_wrapper_t* wrapper_a = &size2_set -> complex_numbers[0];
		complex_wrapper_t* wrapper_b = &size2_set -> complex_numbers[1];
		
		// Pointers for each input
		complex_wrapper_t* even_input = &even_out_set -> complex_numbers[i];
		complex_wrapper_t* odd_input = &odd_out_set -> complex_numbers[i];
		
		wrapper_a -> complex_number = even_input -> complex_number;
		wrapper_b -> complex_number = odd_input -> complex_number;
		
		dft (size2_set, size2_output_set);
		
		// Map our size-2 outputs back out
		output_data -> complex_numbers[i] = size2_output_set -> complex_numbers[0];
		output_data -> complex_numbers[i+half_size-1] = size2_output_set -> complex_numbers[1];
	}
	
}
