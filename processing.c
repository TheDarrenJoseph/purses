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
			fprintf(logfile, "(Output %d/%d) Input Rads: %02f Real: %02f, Imaginary: %02f\n", k, n, rads, real_inc, imag_inc);
			
			// Increment X[k] value
			real_out += real_inc;
			im_out += imag_inc;

		}	
		fprintf(logfile, "(Summation Output %d/%d) Real: %02f, Imaginary: %02f\n", k+1, N, real_out, im_out);
		double complex output = CMPLX(real_out, im_out);
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

complex_set_t* build_complex_set(record_stream_data_t* record_data, complex_set_t** set, int sample_count, int sample_rate) {
		FILE* logfile = get_logfile();
		malloc_complex_set(set, sample_count, sample_rate);

		complex_wrapper_t* data = (*set) -> complex_numbers;
		// Convert samples to Complex numbers
		for (int i=0; i<sample_count; i++) {
			int16_t sample = record_data -> data[i];
			fprintf(logfile, "Read sample (%d) : %d\n", i, sample);
			data[i].complex_number = CMPLX((double) sample, 0.00);
			data[i].magnitude = 0.00;
		}
		return (*set);
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
	build_complex_set(record_stream, &output_set, size_n, SAMPLE_RATE);
}

// Splits data into an N/2 even and odd set
// Performs a DFT on each set
// Recombines the outputs into output_data
void half_size_dfts(complex_set_t* input_data, complex_set_t* output_data) {
	FILE* logfile = get_logfile();
	int sample_rate = input_data -> sample_rate;
	unsigned int half_size = input_data -> data_size/2;

	// 1. Separate input into an N/2 even and odd set
	complex_set_t* even_set = 0;
	complex_set_t* odd_set = 0;
	malloc_complex_set(&even_set, half_size, sample_rate);
	malloc_complex_set(&odd_set, half_size, sample_rate);
	complex_wrapper_t* input_nums = input_data -> complex_numbers;
	for (int i=0; i < half_size; i++) {
		int even_i = i*2;
		int odd_i = i*2+1;		
		fprintf(logfile, "Splitting on Odd: %d, Even: %d\n", odd_i, even_i);
		even_set -> complex_numbers[i] = input_nums[even_i];
		odd_set -> complex_numbers[i] = input_nums[odd_i];
	}
	
	// 2. Perform DFT on each (2 DFTs of size half_size)
	//fprintf(logfile, "=== Performing Odd/Even DFTs ===\n");
	complex_set_t* even_out_set = 0;
	complex_set_t* odd_out_set = 0;
	malloc_complex_set(&even_out_set, half_size, sample_rate);
	malloc_complex_set(&odd_out_set, half_size, sample_rate);
	dft (even_set, even_out_set);
	dft (odd_set, odd_out_set);

	fprintf(logfile, "=== Even Out Data ===\n");
	fprint_data(logfile, even_out_set);
	
	fprintf(logfile, "=== Odd Out Data ===\n");
	fprint_data(logfile, odd_out_set);
	
	// Recombine the split sets
	complex_wrapper_t* output_nums = output_data -> complex_numbers;
	for (int i=0; i < half_size; i++) {
		int even_i = i*2;
		int odd_i = i*2+1;		
		
		double complex even =  even_out_set -> complex_numbers[i].complex_number;
		double complex odd =  odd_out_set -> complex_numbers[i].complex_number;
	
		output_nums[even_i].complex_number = even;
		output_nums[odd_i].complex_number = odd;
		
		fprintf(logfile, "Half-size DFT even result: X[%d] = Real:%02f Imag:%02f\n", even_i, creal(even), cimag(even));
		fprintf(logfile, "Half-size DFT odd result: X[%d]  = Real:%02f Imag:%02f\n", odd_i,  creal(odd), cimag(odd));
	}
}

void wiki_dit2_fft(complex_set_t* input, complex_set_t* output, int size, int start_index, int stride) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "FFT DFT, Size: %d Starting at: %d, Stride: %d\n", size, start_index, stride);		

	int sample_rate = input -> sample_rate;
	unsigned int size_n = size;
	unsigned int half_size = size_n/2;

	// Trivial size-1 DFT 
	if (size_n == 1) {
		fprintf(logfile, "Size-1 DFT, returning: %f\n", input -> complex_numbers[start_index].complex_number);
		output-> complex_numbers[0].complex_number = input -> complex_numbers[start_index].complex_number;
		return;
	}
	
	fprintf(logfile, "Splitting set of size %d into 2 of size %d\n", size_n, half_size);

	// 1. Separate input into an N/2 even and odd set (using stride)
	complex_set_t* first_half = 0;
	complex_set_t* second_half = 0;
	malloc_complex_set(&first_half, half_size, sample_rate);
	malloc_complex_set(&second_half, half_size, sample_rate);
	
	// Map back the split results
	for (int i=0; i<half_size; i++) {
		output -> complex_numbers[start_index+i] = first_half -> complex_numbers[i];
		output -> complex_numbers[start_index+i+half_size] = second_half -> complex_numbers[i];
	}
	
	// Even indexes (0,2,4,...)
	wiki_dit2_fft(input, first_half, half_size, start_index+0, 2);
	// Odd indexes (1,3,5...)
	wiki_dit2_fft(input, second_half, half_size, start_index+1, 2);
	
	for (int k=start_index; k < half_size; k+=stride) {
			fprintf(logfile, "Processing: k=%d, stride=%d\n", k, stride);		
			double rads = -2*M_PI*k/size_n;
			
			double real_twiddle = cos(rads);
			double imag_twiddle = sin(rads);
			double complex twiddle = CMPLX(real_twiddle, imag_twiddle);
			fprintf(logfile, "(Twiddle %d) Input Rads: %02f Real: %02f, Imaginary: %02f\n", k, rads, creal(twiddle), cimag(twiddle));
			
			double complex t = output -> complex_numbers[k].complex_number;
			double complex halfway_offset =  output -> complex_numbers[k+half_size].complex_number;
			output -> complex_numbers[k].complex_number  += t + twiddle * halfway_offset;
			output -> complex_numbers[k+half_size].complex_number += t - twiddle * halfway_offset;
			
			double complex output_k = output -> complex_numbers[k].complex_number;
			double complex output_halfway = output -> complex_numbers[k+half_size].complex_number;
			fprintf(logfile, "X[%d] = Real: %02f Real: %02f, Imaginary: %02f\n", k, creal(output_k), cimag(output_k));
			fprintf(logfile, "X[%d] = Real: %02f Real: %02f, Imaginary: %02f\n", k+half_size, creal(output_halfway), cimag(output_halfway));
	}

	//fprintf(logfile, "=== Output Data ===\n");
	//fprint_data(logfile, output);
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
	print_data(input_data);
	
	complex_set_t* recombined_set = 0;
	malloc_complex_set(&recombined_set, size_n, input_data -> sample_rate);
	half_size_dfts(input_data, recombined_set);
	
	fprintf(logfile, "FFT Processing %d bytes.\n", size_n);
	
	// Twiddle the outputs and recombine
	complex_wrapper_t* recombined_nums = recombined_set -> complex_numbers;
	complex_wrapper_t* output_nums = output_data -> complex_numbers;
	for (int k=0; k < half_size; k++) {
		// Butterfly size-2 DFT Operations
		// Where E is the Even set, O is the Odd set
        	
		// Calculate the Twiddle Factor (e(−2πi k/N))
		// Now for Eueler's formula (for any real number x, given as radians)
		// e^ix = cos(x) + i*sin(x)
		// At this step we are essentially performing our N2 (size-2 DFTs)
		// These are our summations for each of the 2 DFT outputs X[k] and X[k+N/2]
		int even_k = k*2;
		int odd_k = k*2+1;	
		fprintf(logfile, "Recombining Even: %d, Odd:%d\n", even_k, odd_k);

		double rads = -2*M_PI*k/size_n;
			
		double real_twiddle = cos(rads);
		double imag_twiddle = sin(rads);
		double complex twiddle = CMPLX(real_twiddle, imag_twiddle);
		fprintf(logfile, "(Twiddle %d) Input Rads: %02f Real: %02f, Imaginary: %02f\n", k, rads, creal(twiddle), cimag(twiddle));
		
		double complex even = recombined_nums[even_k].complex_number;
		double complex odd = recombined_nums[odd_k].complex_number;
	
		fprintf(logfile, "Recombining k=%d. Even X[%d] = %02f and Odd X[%d] = %02f.\n", k, even_k, creal(even), odd_k, creal(odd));

		double complex t = (twiddle * odd);
		
		// Output -  Xk = E[k] + e(−2πi k/N) O[k]
		double complex xk = even + t;
	
		// Output - Xk+N/2 = E[k] − e(−2πi k/N) O[k]
		double complex xk_plushalf = even - t;
		
		output_nums[k].complex_number += xk;
		output_nums[k+half_size].complex_number += xk_plushalf;
		fprintf(logfile, "=== Output X[%d] Real: %02f, Imaginary: %02f\n", k, creal(xk), cimag(xk));
		fprintf(logfile, "=== Output X[%d] Real: %02f, Imaginary: %02f\n", k+half_size, creal(xk_plushalf), cimag(xk_plushalf));
	}
	
	fprintf(logfile, "=== Output Data ===\n");
	fprint_data(logfile, output_data);
}
