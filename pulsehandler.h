#pragma once

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>

// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_device {
	uint8_t initialized;
	char name[512];
	char monitor_source_name[512];
	uint32_t index;
	char description[256];
} pa_device_t;


typedef struct record_stream_data {
  signed int data[512];
  int data_size;
} record_stream_data_t;

// Shared state for operations/contexts/streams, etc
enum pa_state { 
	// Typically need to iterate the mainloop/await readyness here
	NOT_READY,
	// When we perform our actions upong the op/context/stream, etc
	READY, 
	// General error category
	ERROR, 
	// Point of return
	TERMINATED, 
	// For anything not enumerated/lib changes
	UNKOWN 
	};

void state_cb(pa_context* context, void* userdata);

void print_devicelist(pa_device_t* devices, int size);

void pa_sinklist_cb(pa_context* c, const pa_sink_info* sink_info, 
int eol, void* userdata);

int perform_operation(pa_mainloop* mainloop, pa_context* pa_ctx, 
pa_operation* (*callback) (pa_context* pa_ctx, void* cb_userdata), void* userdata);

int setup_record_stream(const char* device_name, int sink_idx, 
pa_mainloop* mainloop, pa_context* pa_ctx, pa_stream** record_stream, 
enum pa_state* stream_state, record_stream_data_t* stream_read_data);

int perform_read(const char* device_name, int sink_idx, 
pa_mainloop* mainloop, pa_context* pa_ctx, record_stream_data_t* stream_read_data, int* mainloop_retval );

int get_sinklist(pa_device_t* output_devices, int* count);

void init_record_data(record_stream_data_t** stream_read_data);

int record_device(pa_device_t device, 
record_stream_data_t** stream_read_data);
