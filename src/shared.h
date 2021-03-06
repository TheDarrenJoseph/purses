#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <complex.h>

// According to Linux man: console_codes, we want the ANSI ESC char
#define ANSI_DEFAULT "[0;0m"
#define ANSI_RED "[0;31m"
#define ANSI_GREEN "[0;32m"
#define ESC "\033"

#define MAX_ITERATIONS 1000000
#define DEVICE_MAX 16

// 44100Hz sample rate
#define MAX_SAMPLE_RATE 44100
// 43Hz per sample bin
#define NUM_SAMPLES 1024

static const size_t BUFFER_BYTE_COUNT = NUM_SAMPLES;

typedef struct record_stream_data {
  // signed 16-bit integers, size power of 2
  int16_t data[NUM_SAMPLES];
  int data_size;
  bool buffer_filled;
} record_stream_data_t;

// Complex Number

typedef struct complex_wrapper {
  double complex complex_number;
  double magnitude;
  double decibels;
} complex_wrapper_t;

// For use with native complex.h
typedef struct complex_set {
  complex_wrapper_t* complex_numbers;
  // Number of samples in complex_numbers
  int data_size;
  bool has_data;
  // sample rate in Hz
  int sample_rate;
  // The sampling rate of the samples (to allow adjustment)
  int frequency;
} complex_set_t;


FILE* get_logfile();
int close_logfile();
void fprint_data(FILE* file, complex_set_t* samples);
void print_data(complex_set_t* samples);
void fprintln (char* format);
void printlncol(char* ansi_code, char* format);
long seek_file_size(FILE* file);
void write_to_file(record_stream_data_t* stream_read_data, char* filename);
void read_from_file(record_stream_data_t* stream_read_data, char* filename);
