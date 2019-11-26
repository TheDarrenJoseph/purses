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

void pa_state_cb(pa_context *context, void *userdata) {

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

// pa_mainloop will call this function when it's ready to tell us about a sink.
// Since we're not threading, there's no need for mutexes on the devicelist
// structure
void pa_sinklist_cb(pa_context *c, const pa_sink_info *sink_info, int eol, void *userdata) {
	FILE* logfile = get_logfile();
    pa_device_t* pa_devicelist = userdata;
    int ctr = 0;

    // If eol is set to a positive number, you're at the end of the list
    if (eol > 0) {
        return;
    }

    // We know we've allocated 16 slots to hold devices.  Loop through our
    // structure and find the first one that's "uninitialized."  Copy the
    // contents into it and we're done.  If we receive more than 16 devices,
    // they're going to get dropped.  You could make this dynamically allocate
    // space for the device list, but this is a simple example.
    for (ctr = 0; ctr < DEVICE_MAX; ctr++) {
		pa_device_t device = pa_devicelist[ctr];	
		// This is a guard against overwriting fetched devices
        if (!device.initialized) {
            if (strlen(sink_info -> name) > 0) {
				device.index = sink_info -> index;
				strncpy(device.name, sink_info -> name, 511);
				strncpy(device.description, sink_info -> description, 255);
				device.initialized = 1;
				fprintf(logfile, "Found device %d: %d, %s\n", ctr, device.index, device.name);
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
	}
	
	return ERROR;
}

void pa_disconnect(pa_context* pa_ctx, pa_mainloop* mainloop) {
	pa_context_disconnect(pa_ctx);
	pa_context_unref(pa_ctx);
	pa_mainloop_free(mainloop);
}


void get_pa_context(pa_mainloop** mainloop, pa_mainloop_api** pa_mlapi, pa_context** pa_ctx) {
    // Define our pulse audio loop and connection variables
    *mainloop = pa_mainloop_new();
    *pa_mlapi = pa_mainloop_get_api(*mainloop);
    *pa_ctx = pa_context_new(*pa_mlapi, "test"); 
}

pa_operation* get_sink_list(pa_context** pa_ctx, void* userdata) {
	return pa_context_get_sink_info_list(*pa_ctx, pa_sinklist_cb, userdata);
}

int perform_operation(pa_mainloop** mainloop, pa_context** pa_ctx, pa_operation* (*callback) (pa_context** pa_ctx, void* cb_userdata), void* userdata) {
	FILE* logfile = get_logfile();

	 // We'll need these state variables to keep track of our requests
	int pa_ready = 0;
	pa_context_set_state_callback(*pa_ctx, pa_state_cb, &pa_ready);

	bool context_set = false;
	pa_operation* pa_op;
	for (int iterations=0; iterations < MAX_ITERATIONS; iterations++) {		
		//printf("PA Ready state is: %d\n", pa_ready);
		switch (pa_ready) {
			case NOT_READY:
				fprintf(logfile, "PA Not ready, iterating.. %d\n", iterations);
				pa_mainloop_iterate(*mainloop, 1, NULL);
				break;
			case READY:
				if (!context_set) {
					fprintf(logfile, "Getting sink info list...\n");
					//pa_op = pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, output_devices);
					pa_op = (*callback)(pa_ctx, userdata);
					context_set = true;
				} else {
					enum pa_state pa_op_state = check_pa_op(pa_op);
					switch (pa_op_state) {
						case READY:
							fprintf(logfile, "Sink Retrieve Op success.\n");
							pa_disconnect(*pa_ctx, *mainloop);
							return 0;
						case NOT_READY:
							// iterate again
							fprintf(logfile, "Sink Retrieve Op in progress...\n");
							pa_mainloop_iterate(*mainloop, 1, NULL);
							break;
						case ERROR:
							fprintf(logfile, "Sink Retrieve Op failed!\n");
							return 1;	
					}
				}				
				break;
			case ERROR:
				fprintf(logfile, "PA Context encountered an error!\n");
				pa_disconnect(*pa_ctx, *mainloop);
				return 1;
				break;
			default:
				fprintf(logfile, "Unexpected pa_ready state! Exiting.");
				return 1;
				break;

		}
	}
	fprintf(logfile, "Timed out waiting for PulseAudio server ready state!");
	return 1;
}

int pa_get_sinklist(pa_device_t *output_devices) {	
	FILE* logfile = get_logfile();
	fprintf(logfile, "Retrieving PulseAudio Sinks...\n");

    // Make space for 16 devices in our list
    memset(output_devices, 0, sizeof(pa_device_t) * DEVICE_MAX);

    // Define our pulse audio loop and connection variables
    pa_mainloop* mainloop = 0;
    pa_mainloop_api* pa_mlapi = 0;
    pa_context* pa_ctx = 0;
	get_pa_context(&mainloop, &pa_mlapi, &pa_ctx);
	
	pa_context_connect(pa_ctx, NULL, 0 , NULL);

	perform_operation(&mainloop, &pa_ctx, get_sink_list, output_devices);
	
	return 0;
}
