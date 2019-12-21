#include <processing.h>
#include <pulsehandler.h>
#include <shared.h>
#include <math.h>

/**
 * Performs a Discrete Fourier Transform 
 * x - a pointer to a series of complex number inputs
 **/
void dft(complex_n_t* x, complex_n_t* X) {
	
	int N = x -> data_size;
	
	// initialise output
	X = (complex_n_t*) malloc(sizeof(complex_n_t));
	X -> data_size = N;
	
	// Output index (k)
	for (int k=0; k < N; k++) {
		complex_t out_data = X -> data[k];
		
		// Initialise output (Xk)
		double real_out = out_data.real;
		double im_out = out_data.imaginary;
		real_out = 0.00;
		im_out = 0.00;
		
		// Summation over input index (xn)
		for (int n=0; n < N; n++){
			complex_t in_data = x -> data[n];
			double real_xn = in_data.real;
			double im_xn = in_data.imaginary;
			
			// Multiply xn by our complex exponent
			//xn * e^-((i^2*M_PI*k*n)/N)
			
			// Now for Eueler's formula
			// e^xi = cos(x) + i*sin(x)
			// Where i is imaginary
			// x is the angle in radians
			double x = 2*M_PI*k*n / N;
			real_out += (real_xn * cos(x)) + (im_xn * sin(x));
			im_out += (-real_xn * sin(x)) + (im_xn * cos(x));
		}	
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
