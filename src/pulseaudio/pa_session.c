#include <pulseaudio/pa_session.h>

pa_session_t build_session(char* context_name) {
	pa_session_t session = {NULL, NULL, NULL, NULL, NULL, PA_STREAM_UNCONNECTED, NULL};
	// Define our pulse audio loop and connection variables
	session.name = context_name;
	session.mainloop = pa_mainloop_new();
	session.mainloop_api = pa_mainloop_get_api(session.mainloop);
	session.record_stream = NULL;
	session.context = pa_context_new(session.mainloop_api, context_name);
  session.stream_data = NULL;
	return session;
}

// Disconnect the context and the mainloop from the session
void destroy_session(pa_session_t session) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "Destroying PA Session: %s\n", session.name);
	// Disconnect and set the context to NULL
	//disconnect_context(&session.context);
	disconnect_mainloop(&session.mainloop);
	// If we've unininitialised the mainloop, reset the API property also
	if (session.mainloop == NULL) {
		session.mainloop_api = NULL;
	}

	session.stream_data = NULL;
}

void disconnect_context(pa_context** pa_ctx) {
	FILE* logfile = get_logfile();

	if (pa_ctx != NULL && (*pa_ctx) != NULL) {
		pa_context_state_t pa_con_state = pa_context_get_state(*pa_ctx);
		fprintf(logfile, "Disconnecting PA Context with state: %s\n", PA_CONTEXT_STATE_LOOKUP[pa_con_state]);
		if (pa_con_state == PA_CONTEXT_FAILED) {
			fprintf(logfile, "PA Context was already failed or disconnected.\n");
		} else if (pa_con_state == PA_CONTEXT_TERMINATED) {
			fprintf(logfile, "PA Context was already terminated.\n");
		} else {
			fflush(logfile);
			pa_context_disconnect(*pa_ctx);
			*pa_ctx = NULL;
			fprintf(logfile, "Disconnected PA Context!\n");
		}
	} else {
		fprintf(logfile, "Cannot disconnect PA Context. PA Context was NULL!\n");
	}
}

void quit_mainloop(pa_mainloop* mainloop, int retval) {
	if (mainloop != NULL) pa_mainloop_quit(mainloop, retval);
}

void disconnect_mainloop(pa_mainloop** mainloop) {
	FILE* logfile = get_logfile();
	if (mainloop != NULL && *mainloop != NULL) {
		fprintf(logfile, "Disconnecting PA Mainloop...\n");
		fflush(logfile);
		quit_mainloop(*mainloop, 0);
		pa_mainloop_free(*mainloop);
		*mainloop = NULL;
		fprintf(logfile, "Disconnected PA Mainloop!\n");
	} else {
		fprintf(logfile, "Cannot dicsonnect. PA Mainloop was NULL!\n");
	}
}
