#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>

int println (const char *format, va_list vlist) {
	char* output = strcat(format, "\n");
	return vprintf(output, vlist);
}

void print_devicelist(pa_device_t *devices, int size) {
	int ctr=1;
	int found=0;
	for (int i=0; i < size; i++) {
		
		pa_device_t device = devices[ctr];
		int index = device.index;
		char* description = device.description;
		
		if (device.initialized) {
			printw("Initialised? #%d\n", device.initialized);
			printw("Input Device #%d\n", ctr);
			printw("Index: %d\n", device.index);
			// printw("Description: %s\n", description);
			printw("Name: %s\n", device.name);
			printw("\n");
			found = 1;
		}
		ctr++;
	}

	if (!found) printw("No initialised devices found\n");

}

int get_devices() {
  // This is where we'll store the input device list
  //pa_devicelist_t pa_input_devicelist[16];
  
  // This is where we'll store the output device list
  pa_device_t pa_output_devicelist[16];

  int sink_list_stat = pa_get_sinklist(pa_output_devicelist);

  if(sink_list_stat != 0) {
  	printw("Failed to fetch PulseAudio devices!");
	return 1;
  }  else {
	print_devicelist(pa_output_devicelist, 16);
	return 0;
  }
}

int main() {
	// Logfile to handle non-curses output by our handlers
	//FILE* LOGFILE = fopen("purses.log", "w");
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
	get_devices();
	refresh();
	//getch();
	
	delwin(mainwin);

	// register pa_stream_set_write_callback() and pa_stream_set_read_callback() to receive notifications that data can either be written or read. 

	// Create a record stream
	// pa_stream_new for PCM

	//  pa_stream_connect_record()

	// Read with  pa_stream_peek() / pa_stream_drop() for record. Make sure you do not overflow the playback buffers as data will be dropped. 
	
	endwin();
	close_logfile();
	// exit with success status code
	return 0;
}
