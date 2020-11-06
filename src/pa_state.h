#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>

#include <shared.h>

static const char* PA_STATE_LOOKUP[5] = {"NOT_READY", "READY", "ERROR", "TERMINATED", "UNKOWN" };

// Shared state for operations/contexts/streams, etc
typedef enum pa_state {
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
} pa_state_t;


void pa_context_state_cb(struct pa_context* context, void* userdata);
void pa_stream_state_cb(struct pa_stream* stream, void* userdata);
enum pa_state check_pa_op( pa_operation* pa_op);
