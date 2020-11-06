#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>
#include <pa_state.h>
#include <shared.h>

// Signed 16 integer bit PCM, little endian
// Single channel (mono) to ease processing
static const pa_sample_spec mono_ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = SAMPLE_RATE,
	.channels = 1
};

// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_device {
	uint8_t initialized;
	char name[512];
	char monitor_source_name[512];
	uint32_t index;
	char description[256];
} pa_device_t;

typedef struct pa_session {
  // Define our pulse audio loop and connection variables
  pa_mainloop* mainloop;
	pa_mainloop_api* mainloop_api;
  pa_context* context;
} pa_session_t;

void print_devicelist(pa_device_t* devices, int size);
void pa_sinklist_cb(pa_context* c, const pa_sink_info* sink_info, int eol, void* userdata);

int perform_operation(pa_session_t* session, pa_operation* (*callback) (pa_context* pa_ctx, void* cb_userdata), void* userdata);

int setup_record_stream(const char* device_name, int sink_idx,
pa_session_t* session, pa_stream** record_stream,
enum pa_state* stream_state, record_stream_data_t* stream_read_data);

int perform_read(const char* device_name, int sink_idx, pa_session_t* session, record_stream_data_t* stream_read_data, int* mainloop_retval );

pa_session_t build_session(char* context_name);

int get_sinklist(pa_device_t* output_devices, int* count);

void init_record_data(record_stream_data_t** stream_read_data);

int record_device(pa_device_t device, pa_session_t* session, record_stream_data_t** stream_read_data);
