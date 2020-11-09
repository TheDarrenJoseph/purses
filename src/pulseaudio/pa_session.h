#pragma once
// A set of functions and data relating to tracking wrapping the required objects for a recording session with PulseAudio
#include <stdbool.h>
#include <pulse/pulseaudio.h>

#include <shared.h>

static bool BUFFER_FILLED = false;

typedef struct pa_session {
  // Define our pulse audio loop and connection variables
  pa_mainloop* mainloop;
	pa_mainloop_api* mainloop_api;
  pa_context* context;
} pa_session_t;

pa_session_t build_session(char* context_name);
void disconnect_context(pa_context** pa_ctx);
void disconnect_mainloop(pa_mainloop** mainloop);
void destroy_session(pa_session_t session);