#include <pulsehandler.h>

void pa_state_cb(pa_context *context, void *userdata) {

	// assign an int ptr to our void ptr to set type for deref
	enum pa_state* pa_stat = userdata;
        pa_context_state_t pa_con_state = pa_context_get_state(context);
	printf("PA Context State is: %d\n", * pa_stat);
	switch  (pa_con_state) {
		default:
			break;
		case PA_CONTEXT_FAILED:
		case PA_CONTEXT_TERMINATED:
			*pa_stat = NOT_READY;
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
    for (ctr = 0; ctr < 16; ctr++) {
		pa_device_t device = pa_devicelist[ctr];
		
        if (!device.initialized) {
            strncpy(device.name, sink_info -> name, 511);
            strncpy(device.description, sink_info -> description, 255);
            device.index = sink_info -> index;
            device.initialized = 1;
        }
    }
}

int pa_get_devicelist(pa_device_t *output_devices) {
    // Make space for 16 devices in our list
    memset(output_devices, 0, sizeof(pa_device_t) * 16);

    // Define our pulse audio loop and connection variables
    pa_mainloop *mainloop = pa_mainloop_new();
    pa_mainloop_api *pa_mlapi = pa_mainloop_get_api(mainloop);
    pa_context *pa_ctx= pa_context_new(pa_mlapi, "test"); 
	    
    pa_operation *pa_op;
	  
    pa_context_connect(pa_ctx, NULL, 0 , NULL);
    
    // We'll need these state variables to keep track of our requests
    int pa_ready = 0;
    int state = 0;

    // calls out callback with the pa_ready var address
    pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);
    printf("PA Ready state is: %d\n", state);
    switch (state) {
	case NOT_READY:
                printf("Not ready.\n");
 		// This sends an operation to the server.  pa_sinklist_info is
                // our callback function and a pointer to our devicelist will
                // be passed to the callback The operation ID is stored in the
                // pa_op variable
                pa_op = pa_context_get_sink_info_list(pa_ctx,
                        pa_sinklist_cb,
                        output_devices
                        );
		break;
	case READY:
                printf("READY!.\n");
		return 0;
		break;
	default:
		printf("Invalid state: %d, exiting..", state);
		return 1;
		break;
    }
    return 0;
}
