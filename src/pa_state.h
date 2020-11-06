#pragma once
// A set of functions and data relating to tracking the state of various PulseAudio components (Contexts, Streams, etc)

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>

#include <shared.h>
#include <pa_session.h>

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

int await_context_state(pa_session_t* session, pa_state_t expected_state);
int await_stream_ready(pa_session_t* session, pa_stream* stream);
int await_stream_termination(pa_session_t* session, pa_stream* stream, int* mainloop_retval);
int await_operation(pa_mainloop* mainloop, pa_operation* pa_op, pa_context* pa_ctx);
