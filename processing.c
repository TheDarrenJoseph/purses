#include <pulsehandler.h>
#include <shared.h>


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
