#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>
#include <visualiser.h>

void print_devicelist(pa_device_t* devices, int size) {
	FILE* logfile = get_logfile();
	int ctr=1;
	int found=0;
	for (int i=0; i < size; i++) {

		pa_device_t device = devices[i];
		//int index = device.index;
		char* description = device.description;

		if (device.initialized) {
			fprintf(logfile, "=== Device %d, %d ===\n", i+1, device.index);
			fprintf(logfile, "Initialised: %d\n", device.initialized);
			fprintf(logfile, "Input Device: %d\n", ctr);
			fprintf(logfile, "Description: %s\n", description);
			fprintf(logfile, "Name: %s\n", device.name);
			fprintf(logfile, "\n");
			found = 1;
		}
		ctr++;
	}

	if (!found) fprintf(logfile,"No initialised devices found\n");

}

int get_devices(pa_device_t* device_list, int* count) {
  int sink_list_stat = get_sinklist(device_list, count);

  if(sink_list_stat != 0) {
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
	start_color();

	WINDOW* mainwin;
	mainwin = newwin(80,80,1,0);

	// Do stuff
	fprintf(logfile, "===PulseAudio ncurses Visualiser===\n");
	printw(mainwin, "Loading sinks...\n");
	refresh();

	for(int i=0; i<3; i++) {
		// Get devices
		// This is where we'll store the output device list
		pa_device_t output_devicelist[16];

		int count = 0;
		int dev_stat = get_devices(output_devicelist, &count);
		if (dev_stat == 0) print_devicelist(output_devicelist, 16);

		pa_device_t main_device = output_devicelist[0];
		record_stream_data_t* stream_read_data = 0;
		record_device(main_device, &stream_read_data);
		int streamed_data_size = stream_read_data -> data_size;
		fprintf(logfile, "Recorded %d samples\n", streamed_data_size);

		//write_to_file(stream_read_data, "record.bin");

		//record_stream_data_t* file_read_data = 0;
		//init_record_data(&file_read_data);
		//read_from_file(file_read_data, "record.bin");

		complex_set_t* output_set = 0;
		malloc_complex_set(&output_set, streamed_data_size, SAMPLE_RATE);
		complex_set_t* input_set = 0;
		input_set = record_stream_to_complex_set(stream_read_data);
		// And free the struct when we're done
		free(stream_read_data);
		//fflush(logfile);
		//refresh();

		ct_fft(input_set, output_set);
		nyquist_filter(output_set, streamed_data_size);
		set_magnitude(output_set, streamed_data_size);
		printf("=== Result Data ===\n");
		fprint_data(logfile, output_set);

		WINDOW* vis_win;
		vis_win = newwin(VIS_HEIGHT,VIS_WIDTH,1,0);
		draw_visualiser(vis_win, output_set);

		fprintf(logfile, "Iteration %d\n", i);
		mvwprintw(vis_win, 0, 0, "%d", i);
		//wrefresh(mainwin);
		wrefresh(vis_win);

		// Let things catch up
		refresh();
		fflush(logfile);
		//wgetch(vis_win);
		//getch();
		//sleep(5000);
	}

	delwin(mainwin);
	endwin();
	close_logfile();
	// exit with success status code
	return 0;
}
