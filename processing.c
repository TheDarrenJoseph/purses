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
			//fprintf("Adjusting: %.2f, %.2fi by x2 for Nyquist limit\n", realval, imval);
			(*complex_sample) *= 2;
		} else {
			(*complex_sample) = CMPLX(0.0, 0.0);
		}
	}
	
	// Reduce the data size available
	x -> data_size = nyquist_frequency;
}

// Sets the magnitude and decibels for the samples
void set_magnitude(complex_set_t* x, int sample_count) {
	int data_size = x -> data_size;
	for (int i=0; i<data_size; i++) {
		
		struct complex_wrapper* this_val = &(x -> complex_numbers[i]);
		double complex complex_val = this_val -> complex_number;
		
		// Calculate magnitude
		x -> complex_numbers[i].magnitude = (cabs(complex_val) / sample_count);
		// Amplitude in Decibels = 20log10(|m|)
		x -> complex_numbers[i].decibels = 20*log10(x -> complex_numbers[i].magnitude);
	}
}

// x - input set
// X - output set
void dft(complex_set_t* x, complex_set_t* X) {
	FILE* logfile = get_logfile();
	//fprintf(logfile, "Running DFT...\n");
	int N = x -> data_size;
	
	if (N == 1) {
		double complex x0 = x -> complex_numbers[0].complex_number;
		X -> complex_numbers[0].complex_number = x0;
		//fprintf(logfile, "(Output 0/0) Real: %02f, Imaginary: %02f\n", creal(x0), cimag(x0));
		return;
	}
		
	// Output index (k)
	for (int k=0; k < N; k++) {
		//fprintf(logfile, "DFT output (%d)...\n", k);
		
		// Initialise output (X[k])
		double complex output = CMPLX(0.0,0.0);		
		// Summation over input index (x[n]), Multiply x[n] by our complex exponent
		// x[n] * e^-((i^2*M_PI*k*n)/N)
		// Now for Eueler's formula (for any real number x, given as radians)
		// e^ix = cos(x) + i*sin(x)
		// Where e is the base of the natual logarithm
		// Where i is our imaginary number
		// x is the angle in radians (redefined as rads below)
		// Therefore X[k] =	for (int n=0; n < N; n++) { x[n] * (cos(rads) - (i*sin(rads))) }
		for (int n=0; n < N; n++) {
			struct complex_wrapper* in_data = &(x -> complex_numbers[n]);
			double real_xn = creal(in_data -> complex_number);
			double im_xn =  cimag(in_data -> complex_number);
			
			double rads = (2*M_PI/N)*k*n;
			double real_inc = (real_xn * cos(rads)) + (im_xn * sin(rads));
			double imag_inc = (-real_xn * sin(rads)) + (im_xn * cos(rads));
			//fprintf(logfile, "(Output %d/%d) Input Rads: %02f Real: %02f, Imaginary: %02f\n", k, n, rads, real_inc, imag_inc);
			
			// Increment X[k] value
			output += CMPLX(real_inc, imag_inc);
		}	
		//fprintf(logfile, "(Summation Output %d/%d) Real: %02f, Imaginary: %02f\n", k+1, N, creal(output), cimag(output));
		X -> complex_numbers[k].complex_number = output;
	}
}

complex_set_t* malloc_complex_set(complex_set_t** set, int sample_count, int sample_rate) {
		// Initialise the complex samples
		(*set)  = (complex_set_t*) malloc(sizeof(complex_set_t));
		(*set)  -> data_size = sample_count;
		(*set)  -> complex_numbers = (complex_wrapper_t*) malloc(sizeof(complex_wrapper_t) * sample_count);
		return (*set);
}

complex_set_t* build_complex_set(record_stream_data_t* record_data, int sample_count, int sample_rate) {
		FILE* logfile = get_logfile();
		complex_set_t* output_set = 0;
		malloc_complex_set(&output_set, sample_count, sample_rate);
		complex_wrapper_t* data = output_set -> complex_numbers;
		// Convert samples to Complex numbers
		for (int i=0; i<sample_count; i++) {
			int16_t sample = record_data -> data[i];
			//fprintf(logfile, "Read sample (%d) : %d\n", i, sample);
			data[i].complex_number = CMPLX((double) sample, 0.00);
			data[i].magnitude = 0.00;
		}
		return output_set;
}

/**
 * Initialises output_set from the record_stream
 */
complex_set_t* record_stream_to_complex_set(record_stream_data_t* record_stream) {
	FILE* logfile = get_logfile();
	unsigned int size_n = record_stream -> data_size;
	
	int power_of_two = (size_n > 1 && (size_n%2) != 0);
	
	// Check for N power of 2
	if (size_n == 0 || power_of_two) {
		fprintf(logfile, "Cannot perform radix-2 processing if input data size if not a power of 2!, received data size: %d\n", size_n);
		return;
	}
	
	complex_set_t* output_set = 0;
	output_set = build_complex_set(record_stream, size_n, SAMPLE_RATE);
	fprintf(logfile, "Done converting record stream to data set.\n");
	fprintf(logfile, "Data set size: %d\n", output_set -> data_size);
	fprintf(logfile, "Data set sample rate: %dHz\n", SAMPLE_RATE);
	
	return output_set;
}

// Splits data into an N/2 even and odd set
// Performs a DFT on each set
// Recombines the outputs into output_data
void half_size_dfts(complex_set_t* input_data, complex_set_t* output_data, int half_size) {
	FILE* logfile = get_logfile();
	int sample_rate = input_data -> sample_rate;
	
	if (half_size < 1) {
		return;
	}
	//fprintf(logfile, "=== Performing half-size DFTs of size: %d \n", half_size);
	
	// 1. Separate input into an N/2 even and odd set
	complex_set_t* even_set = 0;
	complex_set_t* odd_set = 0;
	malloc_complex_set(&even_set, half_size, sample_rate);
	malloc_complex_set(&odd_set, half_size, sample_rate);
	complex_wrapper_t* input_nums = input_data -> complex_numbers;
	for (int i=0; i < half_size; i++) {
		int even_i = i*2;
		int odd_i = even_i+1;		
		//fprintf(logfile, "Splitting on Odd: %d, Even: %d\n", odd_i, even_i);
		even_set -> complex_numbers[i] = input_nums[even_i];
		odd_set -> complex_numbers[i] = input_nums[odd_i];
	}

	// 2. Perform DFT on each (2 DFTs of size half_size)
	complex_set_t* even_out_set = 0;
	complex_set_t* odd_out_set = 0;
	malloc_complex_set(&even_out_set, half_size, sample_rate);
	malloc_complex_set(&odd_out_set, half_size, sample_rate);
	
	half_size_dfts(even_set, even_out_set, half_size/2);
	half_size_dfts(odd_set, odd_out_set, half_size/2);
	
	//fprintf(logfile, "=== Performing Even-indexed DFT of size: %d\n", half_size);
	dft (even_set, even_out_set);
	//fprintf(logfile, "=== Performing Odd-indexed DFT of size: %d\n", half_size);
	dft (odd_set, odd_out_set);
	
	// Recombine the split sets into the output set
	complex_wrapper_t* output_nums = output_data -> complex_numbers;
	unsigned int size_n = input_data -> data_size;
	//fprintf(logfile, "=== Recombining half-size Even/Odd DFTs of size: %d\n", half_size);
	for (int k=0; k < half_size; k++) {
		// Calculate the Twiddle Factor (e(−2πi k/N))
		// Now for Eueler's formula (for any real number x, given as radians)
		// e^ix = cos(x) + i*sin(x)
		double rads = -2*M_PI*k/size_n;
		// Twiddle factor: e(−2πi k/N) = cos(x) + i*sin(x)
		double complex twiddle = CMPLX(cos(rads), sin(rads));
		double complex even = even_out_set -> complex_numbers[k].complex_number;
		double complex odd  = odd_out_set -> complex_numbers[k].complex_number;
		
		// Twiddle * Odd = e(−2πi k/N) O[k]
		double complex t = (twiddle * odd);
		
		// Output -  Xk = E[k] + e(−2πi k/N) O[k]
		double complex xk = even + t;
		output_nums[k].complex_number = xk;

		// Output - Xk+N/2 = E[k] − e(−2πi k/N) O[k]
		double complex xk_plushalf = even - t;
		output_nums[k+half_size].complex_number = xk_plushalf;
	}
}

// Output will be Audo Frequency (Hz/kHZ) mapped to a rough frequency scale (i.e 1..10); 
// Cooley-Turkey algorithm, radix 2
// This performs a radix-2 via a Decimation In Time (DIT) approach
void ct_fft(complex_set_t* input_data, complex_set_t* output_data) {
	FILE* logfile = get_logfile();
	unsigned int size_n = input_data -> data_size;
	//fprintf(logfile, "=== Performing Radix-2 CT FFT of size: %d \n", size_n);

	// Trivial size-1 DFT 
	if (size_n == 1) {
		output_data -> complex_numbers[0].complex_number = input_data -> complex_numbers[0].complex_number;
		return;
	}
	
	// Split the input into half-size DFTs recursively
	half_size_dfts(input_data, output_data, size_n/2);
}
