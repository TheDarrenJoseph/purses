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

void dft(complex_set_t* x, complex_set_t* X) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "Running DFT...\n");
	int N = x -> data_size;
	// Output index (k)
	for (int k=0; k < N; k++) {
		fprintf(logfile, "DFT output (%d)...\n", k);
		
		// Initialise output (Xk)
		double real_out = 0.0;
		double im_out = 0.0;
		
		// Summation over input index (xn)
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


// Output will be Audo Frequency (Hz/kHZ) mapped to a rough frequency scale (i.e 1..10); 

// Cooley-Turkey algorithm, radix 2
void ct_fft(record_stream_data_t* input_data) {
	FILE* logfile = get_logfile();
	unsigned int size_n = input_data -> data_size;
	
	// Check for N power of 2
	if ((size_n%2) != 0) {
		fprintf(logfile, "Cannot perform radix-2 processing if input data size if not a power of 2!, received data size: %d\n", size_n);
		return;
	}
	
	complex_set_t* input_set = 0;
	build_complex_set(input_data, &input_set, size_n);
	
	fprintf(logfile, "=== Input Data ===\n");
	print_data(input_set, SAMPLE_RATE);
	
	fprintf(logfile, "FFT Processing %d bytes.\n", size_n);
	
	
	complex_set_t* even_set = 0;
	malloc_complex_set(&even_set, size_n/2);
	
	complex_set_t* odd_set = 0;
	malloc_complex_set(&odd_set, size_n/2);
	
	
	complex_wrapper_t* data_even = even_set -> complex_numbers;
	complex_wrapper_t* data_odd = odd_set -> complex_numbers;
	complex_wrapper_t* data_n = input_set -> complex_numbers;
	
	// TODO Implement, for now just show our data
	// 1. DFT even/odd indices
	fprintf(logfile, "FFT Processing even indices.\n");
	for (int i=0; i < size_n; i++) {
		double complex complex_sample = data_n[i].complex_number;
		double realval = creal(complex_sample);
		
		// signed int is at least 16 bits, so we can cast our 16-bit PCM sample into this
		//signed int data_i = (signed int) data_n[i];
		if ((i%2) == 0) {
			fprintf(logfile, "(Even) Processing sample (%d): Real value: %.02f\n", i, realval);
			data_even[i] = data_n[i];
		} else {
			fprintf(logfile, "(Odd) Processing sample (%d): Real value: %.02f\n", i, realval);
			data_even[i] = data_n[i];
		}
	}
	
	//2. 
	
}
