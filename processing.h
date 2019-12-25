#include <pulsehandler.h>
#include <shared.h>
#include <math.h>
#include <float.h>

double magnitude(complex_set_t* input);
void set_magnitude(complex_set_t* x, int sample_count);
//void dft(complex_n_t* x, complex_n_t* X);
void dft(complex_set_t* x, complex_set_t* X);
void ct_fft(record_stream_data_t* input_data);

