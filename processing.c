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
		if (frequency < nyquist_frequency) {
			printf("Adjusting: %.2f\n", creal((*complex_sample)));
			//(*complex_sample) *= 2;
		} else {
			(*complex_sample) = CMPLX(0.0, 0.0);
		}
	}
}

void set_magnitude(complex_set_t* x, int sample_count) {
	int data_size = x -> data_size;
	for (int i=0; i<data_size; i++) {
		
		struct complex_wrapper* this_val = &(x -> complex_numbers[i]);
		double complex complex_val = this_val -> complex_number;
		
		// Calculate magnitude
		//x -> complex_numbers[i].magnitude = ((cabs(complex_val)*2) / sample_count);
		x -> complex_numbers[i].magnitude = ((cabs(complex_val)) / sample_count);
	}
}

void dft(complex_set_t* x, complex_set_t* X) {
	printf("Running DFT...\n");
	int N = x -> data_size;
	// Output index (k)
	for (int k=0; k < N; k++) {
		printf("DFT output (%d)...\n", k);
		
		// Initialise output (Xk)
		double real_out = 0.0;
		double im_out = 0.0;
		
		// Summation over input index (xn)
		for (int n=0; n < N; n++){
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
			printf("(Output %d/%d) Input Rads: %02f Real: %02f, Imaginary: %02f\n", k, n, rads, real_inc, imag_inc);
			real_out += real_inc;
			im_out += imag_inc;

		}	
		printf("(Summation Output %d/%d) Real: %02f, Imaginary: %02f\n", k+1, N, real_out, im_out);
		double complex output = CMPLX(real_out, im_out);
		X -> complex_numbers[k].complex_number = output;
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
	
	fprintf(logfile, "FFT Processing %d bytes.\n", size_n);
	
	// Signed int is atleast 16 bits in size
	int16_t* data_n = input_data -> data;
	
	// TODO Implement, for now just show our data
	// 1. DFT even/odd indices
	fprintf(logfile, "FFT Processing even indices.\n");
	for (int i=0; i < size_n; i++) {
		
		// signed int is at least 16 bits, so we can cast our 16-bit PCM sample into this
		signed int data_i = (signed int) data_n[i];
		if ((i%2) == 0) {
			fprintf(logfile, "(Even) Processing byte (%d): %d\n", i, data_i);
		} else {
			fprintf(logfile, "(Odd) Processing byte (%d): %d\n", i, data_i);
		}
	}
}
