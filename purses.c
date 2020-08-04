#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>

void print_devicelist(pa_device_t* devices, int size) {
	int ctr=1;
	int found=0;
	for (int i=0; i < size; i++) {
		
		pa_device_t device = devices[i];
		//int index = device.index;
		char* description = device.description;
		
		if (device.initialized) {
			printw("=== Device %d, %d ===\n", i+1, device.index);
			printw("Initialised: %d\n", device.initialized);
			printw("Input Device: %d\n", ctr);
			printw("Description: %s\n", description);
			printw("Name: %s\n", device.name);
			printw("\n");
			found = 1;
		} 
		ctr++;
	}

	if (!found) printw("No initialised devices found\n");

}

int get_devices(pa_device_t* device_list, int* count) {
  int sink_list_stat = get_sinklist(device_list, count);
  printw("Retrieved %d Sink Devices...\n", (*count));

  if(sink_list_stat != 0) {
  	printw("Failed to fetch PulseAudio devices!");
	return 1;
  }  else {
	return 0;
  }
}

int main(void) {
	// Logfile to handle non-curses output by our handlers
	FILE* logfile = get_logfile();

	// init curses
	initscr();
	
	WINDOW* mainwin;
	
	mainwin = newwin(80,80,1,0);
	
	// Do stuff
	fprintf(logfile, "===PulseAudio ncurses Visualiser===\n");
	printw("Loading sinks...\n");
	refresh();
	
	// Get devices

	// This is where we'll store the input device list
	//pa_devicelist_t pa_input_devicelist[16];

	// This is where we'll store the output device list
	pa_device_t output_devicelist[16];

	int count = 0;
	int dev_stat = get_devices(output_devicelist, &count);
	if (dev_stat == 0) print_devicelist(output_devicelist, 16);
	
	refresh();
	
	pa_device_t main_device = output_devicelist[0];
	record_stream_data_t* stream_read_data = 0;
	record_device(main_device, &stream_read_data);
	printw("Recording complete...\n");
	
	int streamed_data_size = stream_read_data -> data_size;
	
	//write_to_file(stream_read_data, "record.bin");
	
	//record_stream_data_t* file_read_data = 0;
	//init_record_data(&file_read_data);
	//read_from_file(file_read_data, "record.bin");

	complex_set_t* output_set = 0;
	malloc_complex_set(&output_set, streamed_data_size, sample_rate);
	complex_set_t* input_set = 0;
	record_stream_to_complex_set(stream_read_data, input_set);
	
	//ct_fft(input_set, output_set);
	
	// And free the struct when we're done
	free(stream_read_data);

	refresh();
	//wgetch(mainwin);
	//getch();
	
	delwin(mainwin);
	endwin();
	close_logfile();
	// exit with success status code
	return 0;
}
