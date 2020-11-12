#pragma once
// A set of functions and data relating to tracking wrapping the required objects for a recording session with PulseAudio
#include <stdbool.h>
#include <pulse/pulseaudio.h>

#include <pulseaudio/pa_shared.h>
#include <shared.h>

typedef struct pa_session {
  // Define our pulse audio loop and connection variables
  char* name;
  pa_mainloop* mainloop;
	pa_mainloop_api* mainloop_api;
  pa_context* context;
  pa_stream* record_stream;
  record_stream_data_t* record_stream_data;
} pa_session_t;

pa_session_t build_session(char* context_name);
void disconnect_context(pa_context** pa_ctx);
void quit_mainloop(pa_mainloop* mainloop, int retval);
void disconnect_mainloop(pa_mainloop** mainloop);
void destroy_session(pa_session_t session);
