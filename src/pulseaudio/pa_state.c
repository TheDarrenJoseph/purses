#include <pulseaudio/pa_state.h>

// Handles context state change and sets userdata to our more generic pa_state
void pa_context_state_cb(struct pa_context* context, void* userdata) {
	// assign an int ptr to our void ptr to set type for deref
	enum pa_state* pa_stat = userdata;

  pa_context_state_t pa_con_state = pa_context_get_state(context);
	//printf("PA Context State is: %d\n", (*pa_stat));
	switch  (pa_con_state) {
		// Context setup states
		case PA_CONTEXT_UNCONNECTED:
		case PA_CONTEXT_CONNECTING:
		case PA_CONTEXT_AUTHORIZING:
		case PA_CONTEXT_SETTING_NAME:
			*pa_stat = NOT_READY;
			break;
		// The connection is established
		// the context is ready to execute operations.
		case PA_CONTEXT_READY:
			*pa_stat = READY;
			break;
		// The connection failed or was disconnected.
		case PA_CONTEXT_FAILED:
			*pa_stat = ERROR;
			break;
		// The connection was terminated cleanly.
		case PA_CONTEXT_TERMINATED:
			*pa_stat = TERMINATED;
			break;
		// Anything else/exceptional
		default:
			*pa_stat = UNKOWN;
			break;
	}
}

enum pa_state check_pa_op( pa_operation* pa_op) {
  int pa_op_state = pa_operation_get_state(pa_op);
	switch(pa_op_state){
		case PA_OPERATION_RUNNING:
			return NOT_READY;
		case PA_OPERATION_DONE:
			// Once it's done we can unref/cancel it
			pa_operation_unref(pa_op);
			return TERMINATED;
		case PA_OPERATION_CANCELLED:
			// Once it's done we can unref/cancel it
			pa_operation_unref(pa_op);
			return ERROR;
		default:
			pa_operation_unref(pa_op);
			return UNKOWN;
	}
}

int await_context_state(pa_session_t* session, pa_state_t expected_state) {
	FILE* logfile = get_logfile();

	// This hooks up a callback to set pa_ready/keep this int updated
	int pa_ready = 0;
	pa_context_set_state_callback(session -> context, pa_context_state_cb, &pa_ready);
	for (int i=0; i < MAX_ITERATIONS; i++) {
		//fprintf(logfile, "PA Ready state is: %d\n", pa_ready);
		if (pa_ready == expected_state) {
			fprintf(logfile, "Context reached expected state: %s\n", PA_STATE_LOOKUP[expected_state]);
			return 0;
		} else {
			//fprintf(logfile, "Awaiting context state %s. PA context state is: %s\n", PA_STATE_LOOKUP[expected_state], PA_STATE_LOOKUP[pa_ready]);
			//fflush(logfile);
			switch (pa_ready) {
				case ERROR:
					fprintf(logfile, "PA context encountered an error: %s!\n", pa_strerror(pa_context_errno(session -> context)));
					return 1;
				case TERMINATED:
					// As per docs:
					// " If the connection has terminated by itself,
					// then there is no need to explicitly disconnect
					// the context using pa_context_disconnect()."
					fprintf(logfile, "PA Context is terminated (no longer available)!\n");
					return 1;
				case UNKOWN:
					fprintf(logfile, "Unexpected context state!\n");
					return 1;
			}
			pa_mainloop_iterate(session -> mainloop, 0, NULL);
	 	}
	}
	fprintf(logfile, "Timed out waiting for PulseAudio context state: %s. Actual state was: %s!\n", PA_STATE_LOOKUP[expected_state], PA_STATE_LOOKUP[pa_ready]);
	return 1;
}

pa_state_t convert_stream_state(pa_stream_state_t pa_stream_state) {
	FILE* logfile = get_logfile();
	switch  (pa_stream_state) {
		// The stream is not yet connected to any sink or source.
		case PA_STREAM_UNCONNECTED:
		// The stream is being created.
		case PA_STREAM_CREATING:
			//fprintf(logfile, "Stream unconnected/being created.");
			return NOT_READY;
		//The stream is established, you may pass audio data to it now.
		case PA_STREAM_READY:
			return READY;
		// An error occurred that made the stream invalid.
		case PA_STREAM_FAILED:
			return ERROR;
		// The stream has been terminated cleanly.
		case PA_STREAM_TERMINATED:
			return TERMINATED;
			break;
		// Anything else/exceptional
		default:
			fprintf(logfile, "Unexpected stream state: %d\n", pa_stream_state);
			return UNKOWN;
	}
}

// Handles stream state change and sets userdata to our more generic pa_state
void pa_stream_state_cb(struct pa_stream* stream, void* userdata) {
	// assign an int ptr to our void ptr to set type for deref
	enum pa_state* pa_stat = userdata;
	pa_stream_state_t pa_stream_state = pa_stream_get_state(stream);
	(*pa_stat) = convert_stream_state(pa_stream_state);
}

int await_stream_buffer_filled(pa_session_t* session, pa_stream* stream, int* mainloop_retval) {
	FILE* logfile = get_logfile();
	// Set a callback for when the state changes
	for (int i=0; i < MAX_ITERATIONS; i++) {
		bool buffer_filled = session -> record_stream_data -> buffer_filled;
		if (buffer_filled) {
			fprintf(logfile, "Stream buffer filled.\n");
			fflush(logfile);
			return 0;
		} else {
			pa_mainloop_iterate(session -> mainloop, 0, mainloop_retval);
		}
	}
	fprintf(logfile, "Timed out while waiting for stream buffer to fill.\n");
	return 1;
}

// Iterates the pa_mainloop on the provided session until the stream reaches the expected state
// session - The PulseAudio session
// stream - The PulseAudio recording stream
// expected_state - the expected stream state, this should be either READY, NOT_READY, or TERMINATED
// mainloop_retval - the value returned by iterating the pa_mainloop on the session
// Returns 0 on success, 1 in the event of any issues
int await_stream_state(pa_session_t* session, pa_stream* stream, pa_state_t expected_state, int* mainloop_retval) {
	FILE* logfile = get_logfile();

	// Get the state
	pa_stream_state_t pa_stream_state = pa_stream_get_state(stream);
	pa_state_t stream_state = convert_stream_state(pa_stream_state);
	// Set a callback for when the state changes
	pa_stream_set_state_callback(stream, pa_stream_state_cb, &stream_state);

	const char* expected_state_name = PA_STATE_LOOKUP[expected_state];
	for (int i=0; i < MAX_ITERATIONS; i++) {
		if (stream_state == expected_state) {
			fprintf(logfile, "Stream reached expected state (%s)\n", expected_state_name);
			return 0;
		} else {
			// Handle any errors / unexpected states
			bool buffer_filled = session -> record_stream_data -> buffer_filled;
			const char* state_name = PA_STATE_LOOKUP[stream_state];
			switch (stream_state) {
				case ERROR:
					fprintf(logfile, "PA stream encountered an error: %s!\n", pa_strerror(pa_context_errno(session -> context)));
					return 1;
				case TERMINATED:
					if (buffer_filled) {
						fprintf(logfile, "PA Stream terminated (success)\n");
						return 0;
					} else {
						fprintf(logfile, "PA Stream terminated (failed to fill byte buffer)\n");
						return 1;
					}
				case UNKOWN:
					fprintf(logfile, "Unexpected state! %d\n", stream_state);
          return 1;
			   default:
				  pa_mainloop_iterate(session -> mainloop, 0, mainloop_retval);
					break;
			}
		}
	}
	fprintf(logfile, "Timed out waiting for a PulseAudio stream to reach expected state (%s).\n", expected_state_name);
	return 1;
}

int await_operation(pa_mainloop* mainloop, pa_operation* pa_op, pa_context* pa_ctx) {
	FILE* logfile = get_logfile();

	fprintf(logfile, "Awaiting operation...\n");
	enum pa_state pa_op_state = NOT_READY;
	for (int i=0; i < MAX_ITERATIONS; i++) {
		pa_op_state = check_pa_op(pa_op);
		switch (pa_op_state) {
			case NOT_READY:
				//fprintf(logfile, "Operation in progress... (%d)\n", i);
				// Block until we get something useful
				pa_mainloop_iterate(mainloop, 1, NULL);
				break;
			case ERROR:
				fprintf(logfile, "Operation failed: %s!\n", pa_strerror(pa_context_errno(pa_ctx)));
				return 1;
			case TERMINATED:
				fprintf(logfile, "Operation success.\n");
				return 0;
			default:
				fprintf(logfile, "Unexpected operation state code: %d!\n", pa_op_state);
				return 1;
		}
	}
	fprintf(logfile, "Timed out waiting for a PulseAudio operation to complete.\n");
	return 1;
}
