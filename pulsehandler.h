#pragma once

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>

// Field list is here: http://0pointer.de/lennart/projects/pulseaudio/doxygen/structpa__sink__info.html
typedef struct pa_device {
	uint8_t initialized;
	char name[512];
	uint32_t index;
	char description[256];
} pa_device_t;

enum pa_state { NOT_READY, READY, ERROR };

void pa_state_cb(pa_context* context, void* userdata);

void print_devicelist(pa_device_t* devices, int size);

void pa_sinklist_cb(pa_context* c, const pa_sink_info* sink_info, int eol, void* userdata);

int pa_get_sinklist(pa_device_t* output_devices);

