#pragma once
// A set of functions and data relating to tracking the state of various PulseAudio components (Contexts, Streams, etc)

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <pulse/pulseaudio.h>

#include <shared.h>
#include <pulseaudio/pa_shared.h>
#include <pulseaudio/pa_session.h>

void pa_context_state_cb(struct pa_context* context, void* userdata);
void pa_stream_state_cb(struct pa_stream* stream, void* userdata);
enum pa_state check_pa_op( pa_operation* pa_op);

int await_context_state(pa_session_t* session, pa_state_t expected_state);
int await_stream_buffer_filled(pa_session_t* session, pa_stream* stream, int* mainloop_retval);
int await_stream_state(pa_session_t* session, pa_stream* stream, pa_state_t expected_state, int* mainloop_retval);
int await_operation(pa_mainloop* mainloop, pa_operation* pa_op, pa_context* pa_ctx);
