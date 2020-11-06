#include <pa_state.h>

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

// Handles stream state change and sets userdata to our more generic pa_state
void pa_stream_state_cb(struct pa_stream* stream, void* userdata) {
	FILE* logfile = get_logfile();
	// assign an int ptr to our void ptr to set type for deref
	enum pa_state* pa_stat = userdata;

    pa_stream_state_t pa_stream_state = pa_stream_get_state(stream);
	//fprintf(logfile, "PA Stream State is: %d\n", pa_stream_state);
	switch  (pa_stream_state) {
		// The stream is not yet connected to any sink or source.
		case PA_STREAM_UNCONNECTED:
		// The stream is being created.
		case PA_STREAM_CREATING:
			//fprintf(logfile, "Stream unconnected/being created.");
			(*pa_stat) = NOT_READY;
			break;
		//The stream is established, you may pass audio data to it now.
		case PA_STREAM_READY:
			(*pa_stat )= READY;
			break;
		// An error occurred that made the stream invalid.
		case PA_STREAM_FAILED:
			(*pa_stat) = ERROR;
			pa_stream_disconnect(stream);
			break;
		// The stream has been terminated cleanly.
		case PA_STREAM_TERMINATED:
			fprintf(logfile, "Stream terminated.\n");
			(*pa_stat) = TERMINATED;
			break;
		// Anything else/exceptional
		default:
			(*pa_stat) = UNKOWN;
			pa_stream_disconnect(stream);
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
	fprintf(logfile, "Awaiting context state: %s...\n", PA_STATE_LOOKUP[expected_state]);
	for (int i=0; i < MAX_ITERATIONS; i++) {
		if (pa_ready == expected_state) {
			fprintf(logfile, "Context reached expected state: %s\n", PA_STATE_LOOKUP[expected_state]);
			return 0;
		}
		//fprintf(logfile, "PA Ready state is: %d\n", pa_ready);
		switch (pa_ready) {
			// We can iterate in either of these states
			case NOT_READY:
			case READY:
				//	fprintf(logfile, "Awaiting context setup... (%d)\n", i);
				// Block until we get something useful
				pa_mainloop_iterate(session -> mainloop, 1, NULL);
				break;
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
	}
	fprintf(logfile, "Timed out waiting for PulseAudio server state: %s!\n", PA_STATE_LOOKUP[expected_state]);
	return 1;
}


int await_stream_ready(pa_session_t* session, pa_stream* stream) {
	FILE* logfile = get_logfile();

	enum pa_state stream_state = NOT_READY;
	pa_stream_set_state_callback(stream, pa_stream_state_cb, &stream_state);

	for (int i=0; i < MAX_ITERATIONS; i++) {
		switch (stream_state) {
			case NOT_READY:
				pa_mainloop_iterate(session -> mainloop, 1, NULL);
				break;
			case READY:
				fprintf(logfile, "PA Stream ready.\n");
				return 0;
			case ERROR:
				fprintf(logfile, "PA stream encountered an error: %s!\n", pa_strerror(pa_context_errno(session -> context)));
				return 1;
			case TERMINATED:
				if (BUFFER_FILLED) {
					fprintf(logfile, "PA Stream terminated (success)\n");
					return 0;
				} else {
					fprintf(logfile, "PA Stream terminated (failed to fill byte buffer)\n");
					return 1;
				}
			case UNKOWN:
			default:
				fprintf(logfile, "Unexpected state! %d\n", stream_state);
				return 1;

		}
	}
	fprintf(logfile, "Timed out waiting for a PulseAudio stream to be ready.\n");
	return 1;
}

int await_stream_termination(pa_session_t* session, pa_stream* stream, int* mainloop_retval) {
	FILE* logfile = get_logfile();

	enum pa_state stream_state = NOT_READY;
	pa_stream_set_state_callback(stream, pa_stream_state_cb, &stream_state);

	for (int i=0; i < MAX_ITERATIONS; i++) {
		switch (stream_state) {
			case NOT_READY:
			case READY:
				(*mainloop_retval) = pa_mainloop_iterate(session -> mainloop, 0, NULL);
				break;
			case ERROR:
				fprintf(logfile, "PA stream encountered an error: %s!\n", pa_strerror(pa_context_errno(session -> context)));
				return 1;
			case TERMINATED:
				if (BUFFER_FILLED) {
					fprintf(logfile, "PA Stream terminated (success)\n");
					return 0;
				} else {
					fprintf(logfile, "PA Stream terminated (failed to fill byte buffer)\n");
					return 1;
				}
			case UNKOWN:
			default:
				fprintf(logfile, "Unexpected state! %d\n", stream_state);
				return 1;

		}
	}
	//fprintf(logfile, "Timed out waiting for a PulseAudio stream to terminate.\n");
	//return 1;
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
