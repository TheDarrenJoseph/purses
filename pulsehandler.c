#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>

#include <pulsehandler.h>
#include <shared.h>

#define MAX_ITERATIONS 50000
#define DEVICE_MAX 16

int sink_count = 0;

// Signed 16 integer bit PCM, little endian
// 44100 Hz
// Single channel (mono) to ease processing
static const pa_sample_spec mono_ss = {
	.format = PA_SAMPLE_S16LE,
	.rate = 44100,
	.channels = 1
};

// Handles context state change and sets userdata to our more generic pa_state
void pa_context_state_cb(struct pa_context* context, void* userdata) {
	// assign an int ptr to our void ptr to set type for deref
	enum pa_state* pa_stat = userdata;
     
    pa_context_state_t pa_con_state = pa_context_get_state(context);
	//printf("PA Context State is: %d\n", * pa_stat);
	switch  (pa_con_state) {
		default:
			*pa_stat = NOT_READY;
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_stat = ERROR;
			break;
		case PA_CONTEXT_READY:
			*pa_stat = READY;
			break;
	}	
}


// Handles stream state change and sets userdata to our more generic pa_state
void pa_stream_state_cb(struct pa_stream* stream, void* userdata) {
	// assign an int ptr to our void ptr to set type for deref
	enum pa_state* pa_stat = userdata;
     
    pa_stream_state_t pa_stream_state = pa_stream_get_state(stream);
	//printf("PA Context State is: %d\n", * pa_stat);
	switch  (pa_stream_state) {
		default:
			*pa_stat = NOT_READY;
			break;
		case PA_STREAM_FAILED:
		case PA_STREAM_TERMINATED:
			*pa_stat = ERROR;
			break;
		case PA_STREAM_READY:
			*pa_stat = READY;
			break;
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
			return READY;
		case PA_OPERATION_CANCELLED:
			// Once it's done we can unref/cancel it
			pa_operation_unref(pa_op);
			return ERROR;
		default:
			pa_operation_unref(pa_op);
			return ERROR;
	}
}

void pa_disconnect(pa_context* pa_ctx, pa_mainloop* mainloop) {
	pa_context_disconnect(pa_ctx);
	pa_context_unref(pa_ctx);
	pa_mainloop_free(mainloop);
}


void get_pa_context(char* context_name, pa_mainloop** mainloop, pa_mainloop_api** pa_mlapi, pa_context** pa_ctx) {
    // Define our pulse audio loop and connection variables
    *mainloop = pa_mainloop_new();
    *pa_mlapi = pa_mainloop_get_api(*mainloop);
    *pa_ctx = pa_context_new(*pa_mlapi, context_name); 
}

pa_operation* get_sink_list(pa_context** pa_ctx, void* userdata) {
	return pa_context_get_sink_info_list(*pa_ctx, pa_sinklist_cb, userdata);
}


int await_context_ready(pa_mainloop** mainloop, pa_context** pa_ctx) {
FILE* logfile = get_logfile();

	// This hooks up a callback to set pa_ready/keep this int updated
	int pa_ready = 0;
	pa_context_set_state_callback(*pa_ctx, pa_context_state_cb, &pa_ready);
	for (int i=0; i < MAX_ITERATIONS; i++) {		
		//printf("PA Ready state is: %d\n", pa_ready);
		switch (pa_ready) {
			case NOT_READY:
				fprintf(logfile, "PA Not ready, iterating.. %d\n", i);
				pa_mainloop_iterate(*mainloop, 1, NULL);
				break;
			case READY:
				return 0;
			case ERROR:
				fprintf(logfile, "PA Context encountered an error!\n");
				return 1;
			default:
				fprintf(logfile, "Unexpected pa_ready state! Exiting.\n");
				return 1;

		}
	}
	fprintf(logfile, "Timed out waiting for PulseAudio server ready state!\n");
	return 1;
}

int await_operation(pa_mainloop** mainloop, pa_operation** pa_op) {
	FILE* logfile = get_logfile();
	
	enum pa_state pa_op_state = NOT_READY;
	for (int i=0; i < MAX_ITERATIONS; i++) {		
		pa_op_state = check_pa_op(*pa_op);
		switch (pa_op_state) {
			case READY:
				fprintf(logfile, "Operation success.\n");
				return 0;
			case NOT_READY:
				// iterate again
				fprintf(logfile, "Operation in progress...\n");
				pa_mainloop_iterate(*mainloop, 1, NULL);
				break;
			case ERROR:
				fprintf(logfile, "Operation failed!\n");
				return 1;	
		}
		fflush(logfile);
	}
	fprintf(logfile, "Timed out waiting for a PulseAudio operation to complete.\n");
	return 1;
}

// callback is our implementation function returning pa_operation
int perform_operation(pa_mainloop** mainloop, pa_context** pa_ctx, pa_operation* (*callback) (pa_context** pa_ctx, void* cb_userdata), void* userdata) {
	FILE* logfile = get_logfile();
	
	int await_stat = await_context_ready(mainloop, pa_ctx);
	if (await_stat != 0) {
		fprintf(logfile, "Awaiting PA Context Ready returned failure code: %d\n", await_stat);
		return 1;
	}
	
	bool context_set = false;
	pa_operation* pa_op = 0;
	for (int i=0; i < MAX_ITERATIONS; i++) {		
		if (!context_set) {
			fprintf(logfile, "Performing operation...\n");
			pa_op = (*callback)(pa_ctx, userdata);
			context_set = true;
		} 
		
		if (context_set && pa_op != 0) {
			fprintf(logfile, "Awaiting operation completion...\n");
			int await_op_stat = await_operation(mainloop, &pa_op);
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

void read_stream_cb(pa_stream* read_stream, size_t nbytes, void* userdata) {	
	return;
	FILE* logfile = get_logfile();

	// Print and flush in case this takes time
	fprintf(logfile, "Reading stream...\n");
	fflush(logfile);
	
	// Peek to read each fragment from the buffer
	
	const void** data = 0;
	pa_stream_peek(read_stream, data, &nbytes);
	
	if (data == NULL) {
		if (nbytes == 0) {
			fprintf(logfile, "No data from read stream!...\n");
			return;
		} else {
			// Data hole, call drop to pull it from the buffer 
			// and move the read index forward
			fprintf(logfile, "Read stream data hole, dropping hole data...\n");
			pa_stream_drop(read_stream);
		}
		
	} else {
		fprintf(logfile, "Reading stream of %ld bytes\n", nbytes);
		
		for (long int i=0; i < nbytes; i++) {
			fprintf(logfile, "Reading stream %ld/%ld\n", i, nbytes);
			const int* data_p = (*data);
			// Our format should correspond to signed int of minimum 16 bits
			signed int i_data = data_p[i];
			fprintf(logfile, "-%ld,%d-", i , i_data);
			fflush(logfile);
		}
	}
	
}

void pa_stream_success_cb(pa_stream *stream, int success, void *userdata) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "Stream success: %d", success);
}

int perform_read(const char* device_name, int sink_idx, pa_mainloop** mainloop, pa_context** pa_ctx, int* stream_read_data, int* mainloop_retval ) {
	FILE* logfile = get_logfile();
	
	int await_stat = await_context_ready(mainloop, pa_ctx);
	if (await_stat != 0) {
		fprintf(logfile, "Awaiting PA Context Ready returned failure code: %d\n", await_stat);
		return 1;
	}
	
	const pa_sample_spec * ss = &mono_ss;
	pa_channel_map map;
	pa_channel_map_init_mono(&map);

	// pa_stream_new for PCM
	fprintf(logfile, "Initialising PA Stream for device: %s\n", device_name);
	pa_stream* record_stream = pa_stream_new((*pa_ctx), "purses record stream", ss, &map);
	// register callbacks before connecting
	enum pa_state stream_state = NOT_READY;
	pa_stream_set_state_callback(record_stream, pa_stream_state_cb, &stream_state);
	pa_stream_set_read_callback(record_stream, read_stream_cb, stream_read_data);

	// This connects to the PulseAudio server to read 
	// from the device to our stream
	pa_stream_flags_t stream_flags = PA_STREAM_START_CORKED;
	int monitor_stream_stat = pa_stream_set_monitor_stream(record_stream, sink_idx);
	if (monitor_stream_stat == 0) {
		fprintf(logfile, "Connecting stream for device: %s\n", device_name);
		int connect_stat = pa_stream_connect_record(record_stream, device_name, NULL, stream_flags);
		if (connect_stat < 0) {
			const char* error = pa_strerror(connect_stat); 
			fprintf(logfile, "Failed to connect recording stream for device: %s, status: %d, error: %s\n", device_name, connect_stat, error);
			return 1;
		}
	} else {
		const char* error = pa_strerror(monitor_stream_stat); 
		fprintf(logfile, "Failed to set monitor stream for recording, status: %d,  error: %s\n", monitor_stream_stat, error);
		return 1;
	}
		
	fprintf(logfile, "Starting mainloop to stream device: %s\n", device_name);
	fflush(logfile);
		
	int success_state = 0;
	int mainloop_state = 0;
	while (success_state == 0) {	
		switch (stream_state) {
			case NOT_READY:
				//fprintf(logfile, "PA stream not ready, iterating %d/%d \n", iterations, MAX_ITERATIONS);
				mainloop_state = pa_mainloop_iterate(*mainloop, 0, mainloop_retval);
				break;
			case READY: 
				fprintf(logfile, "PA stream ready..\n");
				// Resume the stream now it's ready
				pa_stream_cork(record_stream, 0, pa_stream_success_cb, mainloop);
				
				for (int i=0; i< MAX_ITERATIONS; i++) {
					fprintf(logfile, "Iterating to read READY stream:  %d/%d \n", i, MAX_ITERATIONS);
					fflush(logfile);
					mainloop_state = pa_mainloop_iterate(*mainloop, 0, mainloop_retval);
				}
			case ERROR:
				fprintf(logfile, "PA stream encountered an error!\n");
				success_state = 1;
			default:
				fprintf(logfile, "Unexpected state! %d\n", stream_state);
				success_state = 1;
		}		
		
		// Mainloop blocks for events on manual iter without us checking this
		if (mainloop_state <= 0) {
			int retval = (*mainloop_retval);
			fprintf(logfile, "Mainloop exited with status: %d \n", mainloop_state);
			pa_stream_disconnect(record_stream);
			return retval;
		} else {
			fprintf(logfile, "Mainloop dispatched %d sources\n", mainloop_state);
		}
		
		fflush(logfile);
	}
	
	//fprintf(logfile, "Timed out waiting for PulseAudio stream ready state!\n");
	pa_stream_disconnect(record_stream);
	return 1;
}

int pa_get_sinklist(pa_device_t* output_devices, int* count) {	
	FILE* logfile = get_logfile();
	fprintf(logfile, "Retrieving PulseAudio Sinks...\n");

    // Make space for 16 devices in our list
    memset(output_devices, 0, sizeof(pa_device_t) * DEVICE_MAX);

    // Define our pulse audio loop and connection variables
    pa_mainloop* mainloop = 0;
    pa_mainloop_api* pa_mlapi = 0;
    pa_context* pa_ctx = 0;
	get_pa_context("sink-list", &mainloop, &pa_mlapi, &pa_ctx);
	
	pa_context_connect(pa_ctx, NULL, 0 , NULL);

	perform_operation(&mainloop, &pa_ctx, get_sink_list, output_devices);
	pa_disconnect(pa_ctx, mainloop);

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

int pa_record_device(pa_device_t device) {	
	FILE* logfile = get_logfile();
	fprintf(logfile, "Recording device: %s\n", device.name);

    // Define our pulse audio loop and connection variables
    pa_mainloop* mainloop = 0;
    int mainloop_retval = 0;
    pa_mainloop_api* pa_mlapi = 0;
    pa_context* pa_ctx = 0;
	get_pa_context("visualiser-pcm-recording", &mainloop, &pa_mlapi, &pa_ctx);
	
	pa_context_connect(pa_ctx, NULL, 0 , NULL);
		
	// Allow 512 bytes back per read
	int* stream_read_data = malloc ( 512 * sizeof(int));
	int read_stat = perform_read(device.monitor_source_name, device.index, &mainloop, &pa_ctx, stream_read_data, &mainloop_retval);
	pa_disconnect(pa_ctx, mainloop);

	if (read_stat == 0) {
		fprintf(logfile, "Recording complete.");
	} else {
		fprintf(logfile, "Recording failed!");
	}
	
	fflush(logfile);
    free(stream_read_data);
	return 0;
}

