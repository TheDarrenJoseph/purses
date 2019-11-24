#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>
#include <pulsehandler.h>

void print_devicelist(pa_device_t *devices, int size) {
int ctr=1;
for (int i=0; i < size; i++) {
	
	int index = devices[ctr].index;
	if (index > 0) {
		printw("Input Device #%d\n", ctr);
		printw("Index: %d\n", devices[ctr].index);
		printw("Description: %s\n", devices[ctr].description);
		printw("Name: %s\n", devices[ctr].name);
		printw("\n");
	}
	ctr++;
}
}

int get_devices() {
  // This is where we'll store the input device list
  //pa_devicelist_t pa_input_devicelist[16];
  
  // This is where we'll store the output device list
  pa_device_t pa_output_devicelist[16];

  int dev_list_stat = pa_get_devicelist(pa_output_devicelist);

  if(dev_list_stat < 0) {
  	printw("Failed to fetch PulseAudio devices!");
	refresh();
	return 1;
  }  else {
	print_devicelist(pa_output_devicelist, 16);
	refresh();
  }
  return 0;
}

int main() {
	// init curses
	initscr();
	
	WINDOW* mainwin;
	
	mainwin = newwin(80,80,1,0);
	
	// Do stuff
	printw("===PulseAudio ncurses Visualiser===\n");
	refresh();
	wgetch(mainwin);


	// Get devices
	return get_devices();
	wgetch(mainwin);
	
	delwin(mainwin);

	// register pa_stream_set_write_callback() and pa_stream_set_read_callback() to receive notifications that data can either be written or read. 

	// Create a record stream
	// pa_stream_new for PCM

	//  pa_stream_connect_record()

	// Read with  pa_stream_peek() / pa_stream_drop() for record. Make sure you do not overflow the playback buffers as data will be dropped. 
	endwin();
	// exit with success status code
	return 0;
}
