#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <complex.h>

#define MAX_ITERATIONS 50000
#define DEVICE_MAX 16

// 44100 Hz sample rate
#define SAMPLE_RATE 512

static const size_t BUFFER_BYTE_COUNT = SAMPLE_RATE;

typedef struct record_stream_data {
  // signed 16-bit integers, size power of 2
  int16_t data[SAMPLE_RATE];
  int data_size;
} record_stream_data_t;

// Complex Number

typedef struct complex_wrapper {
  double complex complex_number;
  double magnitude;
} complex_wrapper_t;

// For use with native complex.h 
typedef struct complex_set {
  complex_wrapper_t* complex_numbers;
  int data_size;
} complex_set_t;


FILE* get_logfile();
int close_logfile();
int println (char* format, va_list vlist);
long seek_file_size(FILE* file);
void write_to_file(record_stream_data_t* stream_read_data, char* filename);
void read_from_file(record_stream_data_t* stream_read_data, char* filename);
