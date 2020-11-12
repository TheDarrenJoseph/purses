#pragma once

extern const char* PA_STATE_LOOKUP[5];
extern const char* PA_CONTEXT_STATE_LOOKUP[7];

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
