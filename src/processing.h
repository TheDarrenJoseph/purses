#include <math.h>
#include <float.h>

#include <pulseaudio/pulsehandler.h>
#include <shared.h>

void nyquist_filter(complex_set_t* x);
double magnitude(complex_set_t* input);
void set_magnitude(complex_set_t* x, int sample_count);
//void dft(complex_n_t* x, complex_n_t* X);
void dft(complex_set_t* x, complex_set_t* X);
complex_set_t* malloc_complex_set(complex_set_t** set, int sample_count, int sample_rate);
complex_set_t*  record_stream_to_complex_set(record_stream_data_t* record_stream);
void ct_fft(complex_set_t* input_data, complex_set_t* output);
