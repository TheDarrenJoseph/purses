#pragma once
#include <pulse/pulseaudio.h>
static bool BUFFER_FILLED = false;

typedef struct pa_session {
  // Define our pulse audio loop and connection variables
  pa_mainloop* mainloop;
	pa_mainloop_api* mainloop_api;
  pa_context* context;
} pa_session_t;
