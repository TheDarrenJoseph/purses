#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>

#include <pulsehandler.h>
#include <shared.h>

/*
// Unsigned 8 Bit PCM for simple byte mapping on read
static const pa_sample_spec mono_ss = {
	.format = PA_SAMPLE_U8,
	.rate = 44100,
	.channels = 1
};*/

static bool BUFFER_FILLED = false;
static bool STREAM_READ_LOCK = false;
static int buffer_nbytes = 0;
static int sink_count = 0;

void quit_mainloop(pa_mainloop* mainloop, int retval) {
	if (mainloop != NULL) pa_mainloop_quit(mainloop, retval);
}

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
	//FILE* logfile = get_logfile();
	int pa_op_state = pa_operation_get_state(pa_op);
	//fprintf(logfile, "PA operation state: %d\n", pa_op_state);

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

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context* c, const pa_sink_info* sink_info, int eol, void* userdata) {
	FILE* logfile = get_logfile();
    pa_device_t* pa_devicelist = userdata;

    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0) {
        return;
    }

    // We know we've allocated 16 slots to hold devices.  Loop through our
    // structure and find the first one that's "uninitialized."  Copy the
    // contents into it and we're done.  If we receive more than 16 devices,
    // they're going to get dropped.  You could make this dynamically allocate
    // space for the device list, but this is a simple example.
    for (int i = 0; i < DEVICE_MAX; i++) {
			pa_device_t device = pa_devicelist[i];
			// This is a guard against overwriting fetched devices
      if (!device.initialized) {
      	if (strlen(sink_info -> name) > 0) {
					device.index = sink_info -> index;
					strncpy(device.name, sink_info -> name, 511);
					strncpy(device.monitor_source_name, sink_info -> monitor_source_name, 511);
					strncpy(device.description, sink_info -> description, 255);
					device.initialized = 1;
					fprintf(logfile, "Found device %d, Index %d, Name: %s\n", i, device.index, device.name);

					if (device.initialized) {
						fprintf(logfile, "Initialized device %d\n", i);
					}
					pa_devicelist[i] = device;
					return;
				}
    	}
    }
}

void pa_disconnect_context(pa_context** pa_ctx) {
	FILE* logfile = get_logfile();

	if ((*pa_ctx) != NULL) {
		pa_context_state_t pa_con_state = pa_context_get_state(*pa_ctx);
		fprintf(logfile, "Disconnecting PA Context with state: %d\n", pa_con_state);
	  if (pa_con_state == PA_CONTEXT_TERMINATED) {
			fprintf(logfile, "PA Context was already terminated.\n");
		} else {
			fprintf(logfile, "Disconnecting PA Context...\n");
			pa_context_disconnect(*pa_ctx);
			*pa_ctx = NULL;
			//pa_context_unref(*pa_ctx);
			fprintf(logfile, "Disconnected PA Context!\n");
		}
	} else {
		fprintf(logfile, "Cannot disconnect. PA Context was NULL!\n");
	}
}

void pa_disconnect(pa_mainloop** mainloop) {
	FILE* logfile = get_logfile();
	if (*mainloop != NULL) {
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

pa_session_t build_session(char* context_name) {
	pa_session_t session = {NULL, NULL, NULL};
	// Define our pulse audio loop and connection variables
	session.mainloop = pa_mainloop_new();
	session.mainloop_api = pa_mainloop_get_api(session.mainloop);
	session.context = pa_context_new(session.mainloop_api, context_name);
	return session;
}

pa_operation* get_sink_list(pa_context* pa_ctx, void* userdata) {
	return pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, userdata);
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

// callback is our implementation function returning pa_operation
int perform_operation(pa_session_t* session, pa_operation* (*callback) (pa_context* pa_ctx, void* cb_userdata), void* userdata) {
	FILE* logfile = get_logfile();

	int await_stat = await_context_state(session, READY);
	if (await_stat != 0) {
		fprintf(logfile, "Awaiting PA Context Ready returned failure code: %d\n", await_stat);
		return 1;
	}

	bool context_set = false;
	pa_operation* pa_op = NULL;
	for (int i=0; i < MAX_ITERATIONS; i++) {
		if (!context_set) {
			fprintf(logfile, "Performing operation...\n");
			pa_op = (*callback)(session -> context, userdata);
			context_set = true;
		}

		if (context_set && pa_op != 0) {
			int await_op_stat = await_operation(session -> mainloop, pa_op, session -> context);

			// I'd like to call pa_unref here, but it seems to always be 0 ref?
			if (await_op_stat != 0) {
				fprintf(logfile, "Awaiting PA Operation returned failure code: %d\n", await_stat);
				return 1;
			} else {
				return 0;
			}
		}
	}
	fprintf(logfile, "Timed out while performing an operation!\n");
	return 1;
}

int read_data(const void* data, record_stream_data_t* data_output, long int buffer_size, size_t* read_bytes) {
	FILE* logfile = get_logfile();
	// Read the nbytes peeked
	//fprintf(logfile, "Reading peeked segment of %ld bytes\n", nbytes);
	//fprintf(logfile, "Reading stream, %ld/%ld bytes\n", read_bytes, initial_nbytes);
	for (long int i=0; i < buffer_size; i++, (*read_bytes)++) {
		//fprintf(logfile, "Reading stream %ld/%ld\n", i+1, BUFFER_BYTE_COUNT);
		const int16_t* data_p = data;
		// Our format should correspond to signed int of minimum 16 bits
		// Set the variable in our output data to match
		data_output -> data[i] = (int16_t) data_p[i];
		//fprintf(logfile, "index: %ld, data: %d\n", i , (signed int) data_p[i]);
	}
	fprintf(logfile, "Read %ld bytes from the stream.\n", BUFFER_BYTE_COUNT);
	return 0;
}

// Waits until the read stream is filled to BUFFER_BYTE_COUNT
// Then reads BUFFER_BYTE_COUNT into our record_stream_data_t for display
void read_stream_cb(pa_stream* read_stream, size_t nbytes, void* userdata) {
	FILE* logfile = get_logfile();

	size_t initial_nbytes = nbytes;

	if (nbytes > 0) {
		if (STREAM_READ_LOCK) {
		   //fprintf(logfile, "Stream already being read..\n");
		   return;
		} else {
			if (nbytes >= BUFFER_BYTE_COUNT) {
				fprintf(logfile, "Read lock..\n");
				STREAM_READ_LOCK = true;
				// Print and flush in case this takes time
				fprintf(logfile, "Reading stream of %ld bytes\n", initial_nbytes);
				size_t total_bytes = 0;
				size_t read_bytes = 0;

				record_stream_data_t* data_output = userdata;
				while (read_bytes < BUFFER_BYTE_COUNT) {
					//pa_stream_readable_size() ???

					// Peek to read each fragment from the buffer, sets nbytes
					const void* data = 0;
					pa_stream_peek(read_stream, &data, &nbytes);

					if (data == NULL) {
						// Data hole, call drop to pull it from the buffer
						// and move the read index forward
						fprintf(logfile, "Read stream data hole, dropping hole data...\n");
						pa_stream_drop(read_stream);
					} else {
						// Read the nbytes peeked
						read_data(data, data_output, BUFFER_BYTE_COUNT, &read_bytes);
					}

					total_bytes += read_bytes;
					//fflush(logfile);
				}

				// Disconnect / Hopefully terminate the stream
				fprintf(logfile, "Read total of %ld / %ld bytes from the stream. Disconnecting..\n", read_bytes, BUFFER_BYTE_COUNT);
				int disconnect_stat = pa_stream_disconnect(read_stream);
				if (disconnect_stat == 0) {
					fprintf(logfile, "Disconnected!\n");
				} else {
					fprintf(logfile, "Error while disconnecting recording stream!\n");
				}
				BUFFER_FILLED = true;
			} else {
				if (nbytes > buffer_nbytes) {
					fprintf(logfile, "Waiting for stream buffer to fill: %ld/%ld.\n", nbytes, BUFFER_BYTE_COUNT);
					buffer_nbytes = nbytes;
				}
			}
		}
	} else {
		fprintf(logfile, "No data to read from stream!...\n");
	}

	fflush(logfile);
}

void pa_stream_success_cb(pa_stream *stream, int success, void *userdata) {
	FILE* logfile = get_logfile();
	if (success) {
		fprintf(logfile, "Stream operation success! retval: %d\n", success);
	} else {
		fprintf(logfile, "Stream operation failed! retval: %d\n", success);
	}
}

int setup_record_stream(const char* device_name, int sink_idx, pa_session_t* session, pa_stream** record_stream, enum pa_state* stream_state, record_stream_data_t* stream_read_data) {
	FILE* logfile = get_logfile();

	const pa_sample_spec * ss = &mono_ss;
	pa_channel_map map;
	fprintf(logfile, "Initialising record stream, sample spec: %d channel(s) @ %dHz\n", ss -> channels, ss -> rate);
	pa_channel_map_init_mono(&map);

	// pa_stream_new for PCM
	fprintf(logfile, "Initialising PA Stream for device: %s\n", device_name);
	(*record_stream) = pa_stream_new(session -> context, "purses record stream", ss, &map);

	// This connects to the PulseAudio server to read
	// from the device to our stream
	int monitor_stream_stat = 0; //pa_stream_set_monitor_stream(record_stream, 0);
	if (monitor_stream_stat == 0) {
		fprintf(logfile, "Connecting stream for device: %s\n", device_name);
		// create pa_buffer_attr to specify stream buffer settings
		const pa_buffer_attr buffer_attribs = {
			// Max buffer size
			.maxlength = (uint32_t) -1,
			// Fragment size, this -1 will let the server choose the size
			// with buffer
			.fragsize = (uint32_t) -1,
		};
		pa_stream_flags_t stream_flags = PA_STREAM_START_CORKED;
		int connect_stat = pa_stream_connect_record(*record_stream, device_name, &buffer_attribs, stream_flags);
		if (connect_stat == 0) {
			fprintf(logfile, "Opened recording stream for device: %s\n", device_name);
		} else {
			const char* error = pa_strerror(connect_stat);
			fprintf(logfile, "Failed to connect recording stream for device: %s, status: %d, error: %s\n", device_name, connect_stat, error);
			return 1;
		}
		return 0;
	} else {
		const char* error = pa_strerror(monitor_stream_stat);
		fprintf(logfile, "Failed to set monitor stream for recording, status: %d,  error: %s\n", monitor_stream_stat, error);
		return 1;
	}
}

int perform_read(const char* device_name, int sink_idx, pa_session_t* session, record_stream_data_t* stream_read_data, int* mainloop_retval) {
	FILE* logfile = get_logfile();

	int await_stat = await_context_state(session, READY);
	if (await_stat != 0) {
		fprintf(logfile, "Awaiting PA Context Ready returned failure code: %d\n", await_stat);
		return 1;
	}

	pa_stream* record_stream = NULL;
	enum pa_state stream_state = NOT_READY;

	int setup_stat = setup_record_stream(device_name, sink_idx, session, &record_stream, &stream_state, stream_read_data);
	if (setup_stat != 0) {
		fprintf(logfile, "Record stream setup failed with return code: %d", setup_stat);
		return 1;
	}

	// await stream readiness
	await_stream_ready(session, record_stream);

	// Set the data read callback now
	pa_stream_set_read_callback(record_stream, read_stream_cb, stream_read_data);

	// Either keep iterating or return
	if (!BUFFER_FILLED) {
		//fprintf(logfile, "PA stream ready\n");
		if (pa_stream_is_corked(record_stream)) {
			fprintf(logfile, "Uncorking stream... \n");
			// Resume the stream now it's ready
			pa_stream_cork(record_stream, 0, pa_stream_success_cb, session -> context);
			pa_mainloop_iterate(session -> mainloop, 1, mainloop_retval);
		}
	}

	fprintf(logfile, "Awaiting filled data buffer from stream: %s\n", device_name);
	fflush(logfile);
	await_stream_termination(session, record_stream, mainloop_retval);
	//fflush(logfile);

	// Success / Failure states
	if (mainloop_retval >= 0) {
		fprintf(logfile, "Mainloop exited sucessfully with value: %d\n", *mainloop_retval);
		return 0;
	} else {
		fprintf(logfile, "Mainloop failed with value: %d\n", *mainloop_retval);
		return 1;
	}
}

int get_sinklist(pa_device_t* output_devices, int* count) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "Retrieving PulseAudio Sinks...\n");

  // Make space for 16 devices in our list
  memset(output_devices, 0, sizeof(pa_device_t) * DEVICE_MAX);

  // Define our pulse audio loop and connection variables
	pa_session_t session = build_session("sink-list");

	pa_context_connect(session.context, NULL, 0 , NULL);
	perform_operation(&session, get_sink_list, output_devices);
	pa_disconnect_context(&session.context);
	pa_disconnect(&session.mainloop);


	int dev_count = 0;
	for (int i=0; i < DEVICE_MAX; i++){

		pa_device_t device = output_devices[i];
		if (device.initialized) {
			dev_count++;
		}
	}
	fprintf(logfile, "Retrieved %d Sink Devices...\n", dev_count);
	(*count) = dev_count;
	return 0;
}


void init_record_data(record_stream_data_t** stream_read_data) {
	// Allocate stuct memory space
	(*stream_read_data) = malloc(sizeof(record_stream_data_t));
	// Allow BUFFER_BYTE_COUNT bytes back per read
	int data_size = (*stream_read_data) -> data_size;
	(*stream_read_data) -> data_size = BUFFER_BYTE_COUNT;
	for (int i=0; i < data_size; i++) {
		(*stream_read_data) -> data[i] = 0;
	}
}

int record_device(pa_device_t device, record_stream_data_t **stream_read_data) {
    FILE *logfile = get_logfile();
    fprintf(logfile, "Recording device: %s\n", device.name);

    int mainloop_retval = 0;
		pa_session_t session = build_session("visualiser-pcm-recording");

    pa_context_connect(session.context, NULL, 0, NULL);
    init_record_data(stream_read_data);

    int read_stat = perform_read(device.monitor_source_name, device.index, &session, (*stream_read_data),
                                 &mainloop_retval);

    if (read_stat == 0) {
        fprintf(logfile, "Recording complete.\n");
        //(*buffer_size) = BUFFER_BYTE_COUNT;
        // Display our data?
    } else {
        fprintf(logfile, "Recording failed!\n");
				return 1;
        //(*buffer_size) = 0;
    }

    // Segfaulting
		//pa_disconnect_context(&session.context);
		pa_disconnect(&session.mainloop);
    fflush(logfile);
    return 0;
}