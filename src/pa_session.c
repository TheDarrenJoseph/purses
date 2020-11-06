#include <pa_session.h>

pa_session_t build_session(char* context_name) {
	pa_session_t session = {NULL, NULL, NULL};
	// Define our pulse audio loop and connection variables
	session.mainloop = pa_mainloop_new();
	session.mainloop_api = pa_mainloop_get_api(session.mainloop);
	session.context = pa_context_new(session.mainloop_api, context_name);
	return session;
}

void disconnect_context(pa_context** pa_ctx) {
	FILE* logfile = get_logfile();

	if (pa_ctx != NULL && (*pa_ctx) != NULL) {
		pa_context_state_t pa_con_state = pa_context_get_state(*pa_ctx);
		fprintf(logfile, "Disconnecting PA Context with state: %d\n", pa_con_state);
	  if (pa_con_state == PA_CONTEXT_TERMINATED) {
			fprintf(logfile, "PA Context was already terminated.\n");
		} else {
			fprintf(logfile, "Disconnecting PA Context...\n");
			pa_context_disconnect(*pa_ctx);
			*pa_ctx = NULL;
			fprintf(logfile, "Disconnected PA Context!\n");
		}
	} else {
		fprintf(logfile, "Cannot disconnect. PA Context was NULL!\n");
	}
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

// Disconnect the context and the mainloop from the session
void destroy_session(pa_session_t session) {
	if (session.context != NULL) {
		// Disconnect and set the context to NULL
		disconnect_context(&session.context);
	}

	if (session.mainloop != NULL) {
		disconnect_mainloop(&session.mainloop);
		session.mainloop_api = NULL;
	}
}
