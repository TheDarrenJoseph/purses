#include <pulseaudio/pulsehandler.h>

/*
// Unsigned 8 Bit PCM for simple byte mapping on read
static const pa_sample_spec mono_ss = {
	.format = PA_SAMPLE_U8,
	.rate = 44100,
	.channels = 1
};*/

static bool STREAM_READ_LOCK = false;
static int buffer_nbytes = 0;

void pa_sinklist_cb(pa_context* c, const pa_sink_info* sink_info, int eol, void* userdata) {
	FILE* logfile = get_logfile();
    pa_device_t* pa_devicelist = userdata;

    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0) return;

    // We know we've allocated 16 slots to hold devices.  Loop through our
    // structure and find the first one that's "uninitialized."  Copy the
    // contents into it and we're done.  If we receive more than 16 devices,
    // they're going to get dropped.
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
					if (device.initialized)	fprintf(logfile, "Initialized device %d\n", i);
					pa_devicelist[i] = device;
					return;
				}
    	}
    }
}

pa_operation* get_sink_list(pa_context* pa_ctx, void* userdata) {
	return pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, userdata);
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
			}
			return await_op_stat;
		}
	}
	fprintf(logfile, "Timed out while performing an operation!\n");
	return 1;
}

int read_data(const void* data, record_stream_data_t* data_output, long int buffer_size, size_t* read_bytes) {
	for (long int i=0; i < buffer_size; i++, (*read_bytes)++) {
		const int16_t* data_p = data;
		// Our format should correspond to signed int of minimum 16 bits
		// Set the variable in our output data to match
		data_output -> data[i] = (int16_t) data_p[i];
	}
	return 0;
}

int disconnect_stream(pa_stream* stream) {
	FILE* logfile = get_logfile();
	pa_stream_state_t pa_stream_state = pa_stream_get_state(stream);

	// We should only really disconnect a stream if it's not failed or disconnected
  if (pa_stream_state == PA_STREAM_FAILED || pa_stream_state == PA_STREAM_TERMINATED) {
		fprintf(logfile, "Will not disconnect stream, stream is already failed/terminated with state code: %d!\n", pa_stream_state);
		return 1;
	} else {
		int disconnect_stat = pa_stream_disconnect(stream);
		if (disconnect_stat == 0) {
			fprintf(logfile, "Stream disconnected!\n");
		} else {
			fprintf(logfile, "Error while disconnecting stream, disconnect failed with code: %d!\n", disconnect_stat);
		}
		return disconnect_stat;
	}
}

// pa_stream_request_cb_t
// Waits until the read stream is filled to BUFFER_BYTE_COUNT
// Then reads BUFFER_BYTE_COUNT into our record_stream_data_t for display
void read_stream_cb(pa_stream* p, size_t nbytes, void* userdata) {

  pa_session_t* session = userdata;
  bool buffer_filled = session -> stream_data -> buffer_filled;
  if (buffer_filled) {
      return;
  }
    
  FILE* logfile = get_logfile();
	if (nbytes > 0) {
		if (STREAM_READ_LOCK) {
		   fprintf(logfile, "Stream already being read. Cancelling stream read callback.\n");
		   return;
		} else if (session -> stream_data -> buffer_filled) {
       fprintf(logfile, "Buffer already filled. Cancelling stream read callback.\n");
		   return;
    } else {
				fprintf(logfile, "Stream read locked.\n");
				STREAM_READ_LOCK = true;
        fprintf(logfile, "Initial buffer size: %d / %ld\n", session -> stream_data -> data_size, BUFFER_BYTE_COUNT);
        long int stream_byte_size = nbytes;
				fprintf(logfile, "Reading stream of %ld bytes\n", stream_byte_size);
				size_t total_read_bytes = 0;
				while (total_read_bytes < stream_byte_size) {
					// Peek to read each fragment from the buffer, sets nbytes
					const void* data = 0;
					pa_stream_peek(p, &data, &nbytes);

          //fprintf(logfile, "Stream peek returned fragment of %ld bytes\n", nbytes);
          int buffer_size = session -> stream_data -> data_size;
          int remaining_buffer_bytes = BUFFER_BYTE_COUNT - buffer_size;
          size_t read_bytes = 0;
          if (remaining_buffer_bytes > 0 && data != NULL && !session -> stream_data -> buffer_filled) {
            // If we have enough bytes (nbytes) to fill what's remaining, read that
            // Otherwise read whatever is available 
            int bytes_to_read =  (nbytes >= remaining_buffer_bytes) ? remaining_buffer_bytes : nbytes;
            //fprintf(logfile, "Reading %d out of peeked fragment of %ld bytes\n", bytes_to_read, nbytes);
            read_data(data, session -> stream_data, bytes_to_read, &read_bytes);
            fprintf(logfile, "Read %ld bytes from the stream buffer\n", read_bytes);
            session -> stream_data -> data_size += read_bytes;
            total_read_bytes += read_bytes;
          } else if (data != NULL && session -> stream_data -> buffer_filled) {
            fprintf(logfile, "Further stream data beyond buffer capacity, dropping fragment of %ld bytes\n", nbytes);
            pa_stream_drop(p);
            total_read_bytes += nbytes;
          } else if (data == NULL){
            // Data hole, call drop to pull it from the buffer
            // and move the read index forward
            fprintf(logfile, "Read stream data hole, dropping fragment of %ld bytes\n", nbytes);
            pa_stream_drop(p);
            total_read_bytes += nbytes;
          } 

          // Set the buffer filled flag if we need to
          if (buffer_size == BUFFER_BYTE_COUNT && !session -> stream_data -> buffer_filled ) {
            fprintf(logfile, "DONE filling stream read buffer.\n");
            session -> stream_data -> buffer_filled  = true;
          } 
        }
        fprintf(logfile, "DONE reading a total of %ld / %ld bytes from the stream.\n", total_read_bytes, stream_byte_size);
        fprintf(logfile, "Final buffer size: %d / %ld\n", session -> stream_data -> data_size, BUFFER_BYTE_COUNT);
				fprintf(logfile, "Stream read unlocked.\n");
        STREAM_READ_LOCK = false;
		}
	} else {
		fprintf(logfile, "No data to read from stream!\n");
	}
}

void pa_stream_success_cb(pa_stream *stream, int success, void *userdata) {
  char* descriptor = (char*) userdata;
	FILE* logfile = get_logfile();
	if (success) {
		fprintf(logfile, "Stream operation '%s' was a success! retval: %d\n", descriptor, success);
	} else {
		fprintf(logfile, "Stream operation '%s' failed! retval: %d\n", descriptor, success);
	}
}

int connect_record_stream(pa_stream** record_stream, const char* device_name) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "Connecting stream for device: %s\n", device_name);
	int connect_stat = pa_stream_connect_record(*record_stream, device_name, &buffer_attribs, PA_STREAM_START_CORKED);
	if (connect_stat == 0) {
		fprintf(logfile, "Opened recording stream for device: %s\n", device_name);
	} else {
		const char* error = pa_strerror(connect_stat);
		fprintf(logfile, "Failed to connect recording stream for device: %s, status: %d, error: %s\n", device_name, connect_stat, error);
		return 1;
	}
	return 0;
}

pa_stream* setup_record_stream(pa_session_t* session) {
	FILE* logfile = get_logfile();
	const pa_sample_spec * ss = &mono_ss;
	pa_channel_map map;
	fprintf(logfile, "Initialising record stream, sample spec: %d channel(s) @ %dHz\n", ss -> channels, ss -> rate);
	pa_channel_map_init_mono(&map);

	// pa_stream_new for PCM
	fprintf(logfile, "Initialising PA recording Stream.\n");
	pa_stream* record_stream = pa_stream_new(session -> context, "purses record stream", ss, &map);
	return record_stream;
}

int cork_stream(pa_stream* stream, pa_session_t* session, int* mainloop_retval) {
	if (!pa_stream_is_corked(stream)) {
		// Resume the stream now it's ready
		pa_stream_cork(stream, 1, pa_stream_success_cb, "cork");
		pa_mainloop_iterate(session -> mainloop, 1, mainloop_retval);
		return 0;
	}
	return 1;
}

int uncork_stream(pa_stream* stream, pa_session_t* session, int* mainloop_retval) {
	if (pa_stream_is_corked(stream)) {
		// Resume the stream now it's ready
		pa_stream_cork(stream, 0, pa_stream_success_cb, "uncork");
		pa_mainloop_iterate(session -> mainloop, 1, mainloop_retval);
		return 0;
	}
	return 1;
}

int perform_read(const char* device_name, int sink_idx, pa_session_t** s) {
	FILE* logfile = get_logfile();
	int mainloop_retval = 0;
	
  pa_session_t* session = *s;

	if (session -> record_stream == NULL) {
		 session -> record_stream = setup_record_stream(session);
	}
  
  pa_stream* record_stream = session -> record_stream;
  pa_stream_state_t stream_state = session -> stream_state;
  if (stream_state == PA_STREAM_UNCONNECTED) {
    int connection_state = connect_record_stream(&record_stream, device_name);
    if (connection_state == 0) {
      fprintf(logfile, "Record stream connected.\n");
    } else {
      fprintf(logfile, "Record stream connection failed with return code: %d\n", connection_state);
      return 1;
    }
  }

	await_stream_state(session, record_stream, READY, &mainloop_retval);
	// Reset the byte count for the buffer
	buffer_nbytes = 0;

	fprintf(logfile, "Awaiting filled data buffer from stream: %s\n", device_name);
	// Set the data read callback now
	pa_stream_set_read_callback(record_stream, read_stream_cb, session);
	uncork_stream(record_stream, session, &mainloop_retval);
	int await_stat = await_stream_buffer_filled(session, record_stream, &mainloop_retval);
	if (await_stat != 0) {
		fprintf(logfile, "Something went wrong while waiting for a stream to terminate!\n");
	  return 1;
	}

	// Drain whatever is left in the stream now we've pulled it onto the buffer
  cork_stream(record_stream, session, &mainloop_retval);
	pa_stream_drain(record_stream, pa_stream_success_cb, NULL);
	pa_mainloop_iterate(session -> mainloop, 1, &mainloop_retval);
  
  // Re-assign the ouput
  *s = session;
	// Success / Failure states
	if (mainloop_retval >= 0) {
		fprintf(logfile, "Mainloop exited sucessfully with value: %d\n", mainloop_retval);
		return 0;
	} else {
		fprintf(logfile, "Mainloop failed with value: %d\n", mainloop_retval);
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
	destroy_session(session);

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

// Either initialise or empty the record data object for use
void clean_stream_data(record_stream_data_t** stream_data) {
    FILE *logfile = get_logfile();
    // Temporary pointer value
    record_stream_data_t* record_data = (*stream_data);

		if (record_data == NULL) {
      fprintf(logfile, "Allocating record stream data\n");
      record_data = malloc(sizeof(record_stream_data_t));
		}
    for (int i=0; i < record_data -> data_size; i++) {
      record_data -> data[i] = 0;
    }
    record_data -> data_size = 0;
    record_data -> buffer_filled = false;
    (*stream_data) = record_data;
    fprintf(logfile, "Cleaned record stream data of %d bytes\n", record_data -> data_size);
}

int record_device(pa_device_t device, pa_session_t** s) {
    FILE *logfile = get_logfile();
    fprintf(logfile, "Recording device: %s\n", device.name);
		fflush(logfile);

    pa_session_t* session = *s;

    clean_stream_data(&session -> stream_data);

		pa_context_state_t pa_con_state = pa_context_get_state(session -> context);
		if (PA_CONTEXT_UNCONNECTED == pa_con_state) {
    	pa_context_connect(session -> context, NULL, 0, NULL);
		}
 		pa_con_state = pa_context_get_state(session -> context);
		if (PA_CONTEXT_READY != pa_con_state) {
			int await_stat = await_context_state(session, READY);
			if (await_stat != 0) {
				fprintf(logfile, "Awaiting PA Context Ready returned failure code: %d\n", await_stat);
				return 1;
			}
		}

    int read_stat = perform_read(device.monitor_source_name, device.index, &session);
    if (read_stat == 0) {
        fprintf(logfile, "Recording complete. Recorded %d samples\n", session -> stream_data -> data_size);
    } else {
        fprintf(logfile, "Recording failed!\n");
    }
    *s = session;
		return read_stat;
}
