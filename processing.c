#include <pulsehandler.h>
#include <shared.h>

// Cooley-Turkey algorithm
void ct_fft(record_stream_data_t* input_data) {
	FILE* logfile = get_logfile();
	unsigned int size_n = input_data -> data_size;
	
	fprintf(logfile, "FFT Processing %d bytes.\n", size_n);
	
	// Signed int is atleast 16 bits in size
	unsigned int* data_n = input_data -> data;

	for (int i=0; i < size_n; i++) {
		unsigned int data_i = data_n[i];
		fprintf(logfile, "Processing byte (%d): %u\n", i, data_i);
	}
}
