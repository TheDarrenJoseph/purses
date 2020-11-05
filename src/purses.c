#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>
#include <visualiser.h>

// Prints a PulseAudio device to the logfile
void print_device(pa_device_t device, int device_index) {
	FILE* logfile = get_logfile();
	fprintf(logfile, "=== Device %d, %d ===\n", device_index, device.index);
	fprintf(logfile, "Initialised: %d\n", device.initialized);
	fprintf(logfile, "Input Device: %d\n", device_index + 1);
	fprintf(logfile, "Description: %s\n", device.description);
	fprintf(logfile, "Name: %s\n", device.name);
	fprintf(logfile, "\n");
}

// Loops through the provided devices (using size to iterate)
// Printing each initialised device to the logfile
void print_devicelist(pa_device_t* devices, int size) {
	FILE* logfile = get_logfile();
	int found=0;
	for (int i=0; i < size; i++) {
		pa_device_t device = devices[i];
		if (device.initialized) {
			print_device(device, i);
			found = 1;
		}
	}
	if (!found) fprintf(logfile,"No initialised devices found\n");
}

// Gets a list of PulseAudio Sinks
// Returns 0 if the listing was successful, 1 in the event of an error
int get_sinks(pa_device_t* device_list, int* count) {
  int sink_list_stat = get_sinklist(device_list, count);
	return (sink_list_stat != 0) ? 1 : 0;
}

pa_device_t get_main_device() {
	pa_device_t output_devicelist[16];
	int count = 0;
	int dev_stat = get_sinks(output_devicelist, &count);
	if (dev_stat == 0) print_devicelist(output_devicelist, 16);
	pa_device_t main_device = output_devicelist[0];
	return main_device;
}

// Writes the recorded stream data to a binary file called "record.bin"
void write_stream_to_file(record_stream_data_t* stream_read_data) {
	write_to_file(stream_read_data, "record.bin");
}

// Reads recorded stream data from the binary file called "record.bin"
record_stream_data_t* read_stream_from_file() {
	record_stream_data_t* file_read_data = 0;
	init_record_data(&file_read_data);
	read_from_file(file_read_data, "record.bin");
	return file_read_data;
}

// Records a set amount of data from the device
// Returns a record_stream_data_t filled from the device on successful
// Returns NULL in the event of a failure
record_stream_data_t* record_samples_from_device(pa_device_t device) {
	record_stream_data_t* stream_read_data = 0;
	int recording_stat = record_device(device, &stream_read_data);
	if (recording_stat == 0) {
		return stream_read_data;
	} else {
		return NULL;
	}
}

int main(void) {
	// Logfile to handle non-curses output by our handlers
	FILE* logfile = get_logfile();

	// init curses
	initscr();
	start_color();
	refresh();

	WINDOW* vis_win = newwin(VIS_HEIGHT,VIS_WIDTH,1,0);
	pa_device_t device = get_main_device();
	for(int i=0; i<2; i++) {
		record_stream_data_t* stream_read_data = record_samples_from_device(device);
		if (stream_read_data != NULL) {
			int streamed_data_size = stream_read_data -> data_size;
			fprintf(logfile, "Recorded %d samples\n", streamed_data_size);

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
			fprintf(logfile, "=== Result Data ===\n");
			fprint_data(logfile, output_set);
			draw_visualiser(vis_win, output_set);
		} else {
			fprintf(logfile, "Failed to record samples from device.\n");
		}

		fprintf(logfile, "Iteration %d\n", i);
		mvwprintw(vis_win, 0, 0, "%d", i);
		//wrefresh(mainwin);
		wrefresh(vis_win);

		// Let things catch up
		refresh();
		fflush(logfile);
		wgetch(vis_win);
		//getch();
		//sleep(5000);
	}

	delwin(vis_win);
	endwin();
	close_logfile();
	// exit with success status code
	return 0;
}
