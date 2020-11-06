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
