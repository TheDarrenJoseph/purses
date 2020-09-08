#include <pulsehandler.h>
#include <shared.h>
#include <math.h>
#include <float.h>

double magnitude(complex_set_t* input);
void set_magnitude(complex_set_t* x, int sample_count);
//void dft(complex_n_t* x, complex_n_t* X);
void dft(complex_set_t* x, complex_set_t* X);
complex_set_t* record_stream_to_complex_set(record_stream_data_t* record_stream);
void ct_fft(complex_set_t* input_data, complex_set_t* output);

