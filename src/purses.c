#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>
#include <time.h>

#include <pulseaudio/pulsehandler.h>
#include <shared.h>
#include <processing.h>
#include <visualiser.h>
#include <settings.h>

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



// Writes the recorded stream data to a binary file called "record.bin"
void write_stream_to_file(record_stream_data_t* stream_read_data) {
	write_to_file(stream_read_data, "record.bin");
}

// Reads recorded stream data from the binary file called "record.bin"
record_stream_data_t* read_stream_from_file() {
	record_stream_data_t* file_read_data = malloc(sizeof(record_stream_data_t));
	read_from_file(file_read_data, "record.bin");
	return file_read_data;
}

// Records a set amount of data from the device
// Returns a record_stream_data_t filled from the device on successful
// Returns NULL in the event of a failure
record_stream_data_t* record_samples_from_device(pa_device_t device, pa_session_t* session) {
	int recording_stat = record_device(device, &session);
	if (recording_stat == 0) {
		return session -> stream_data;
	} else {
		return NULL;
	}
}

// Records some samples from the provided device
// Performing a Cooley-Tukey FFT on the recording
// Then drawing the visualiser graph for the results
void perform_visualisation(pa_device_t* device, pa_session_t* session, WINDOW* vis_win) {
	FILE* logfile = get_logfile();
	struct timeval before, after, elapsed;
	gettimeofday(&before, NULL);

	record_stream_data_t* stream_data = record_samples_from_device(*device, session);
	complex_set_t* output_set = NULL;
	if (stream_data != NULL) {
		int streamed_data_size = stream_data -> data_size;
    if (stream_data -> buffer_filled) {
      complex_set_t* input_set = NULL;
      input_set = record_stream_to_complex_set(stream_data);
      // And free the struct when we're done
      free(stream_data);
      //fflush(logfile);
      //refresh();
      malloc_complex_set(&output_set, streamed_data_size, MAX_SAMPLE_RATE);
      fprintf(logfile, "=== Recorded Data ===\n");
      fprint_data(logfile, input_set);

      ct_fft(input_set, output_set);
      nyquist_filter(output_set);
      set_magnitude(output_set, streamed_data_size);
      fprintf(logfile, "=== Result Data ===\n");
      fprint_data(logfile, output_set);
    }
	} else {
		// Zero the output set so we can display nothing
		malloc_complex_set(&output_set, 0, MAX_SAMPLE_RATE);
		fprintf(logfile, "Failed to record samples from device.\n");
	}

	gettimeofday(&after, NULL);
	// Set the subtracted elapsed time
	timersub(&after, &before, &elapsed);
	draw_visualiser(vis_win, output_set, elapsed);
	mvwprintw(vis_win, VIS_HEIGHT-1, 1, "q - Quit, s - Choose device");
	wrefresh(vis_win);
	refresh();
}

// Gets a single character of input from the provided window
// Returning an integer of 0 if nothing is matched
// 1 for quit
// 2 for settings menu
int handle_input(WINDOW* window) {
	int keypress = wgetch(window);
	if (ERR != keypress) {
		char input = (char) keypress;
    switch(input) {
      case 'q':
        return 1;
      case 's':
        return 2;
    } 
	}
	return 0;
}

int main(void) {
	FILE* logfile = get_logfile();
  const char* TESTING_MODE_ENV = getenv("PURSES_TEST_MODE");
  const bool TESTING_MODE = TESTING_MODE_ENV != NULL && TESTING_MODE_ENV[0] == '1';
	// init curses
	initscr();
	start_color();
	refresh();
  // Don't write input characters to the display
  noecho();

	WINDOW* visusaliser_win = newwin(VIS_HEIGHT, VIS_WIDTH, 1, 0);
	WINDOW* settings_win = newwin(SETTINGS_HEIGHT, SETTINGS_WIDTH, 2, 5);

  // The delay for reading from a window (use a large value to step through each iteration)
  const int READ_TIMEOUT_MILIS = TESTING_MODE ? 60000 : 100;
	wtimeout(visusaliser_win, READ_TIMEOUT_MILIS);
	wtimeout(settings_win, 500);
	pa_device_t device = get_main_device();
  int device_index = 0;
	pa_session_t session = build_session("visualiser-pcm-recording");
  unsigned long int i = 0;
	while (true) {
		fprintf(logfile, "=== Performing visualisation frame no: %ld\n", i);
		perform_visualisation(&device, &session, visusaliser_win);
		// Print the current iteration count
    if(TESTING_MODE) mvwprintw(visusaliser_win, 0, 0, "%ld", i);
		fflush(logfile);
		int command_code = handle_input(visusaliser_win);
		if (command_code == 1) break;
		if (command_code == 2) {
      device = show_device_choice_window(settings_win, &device_index);
  		fprintf(logfile, "=== Chosen device: %d. %s\n", device_index, device.name);
      werase(visusaliser_win);
      wrefresh(visusaliser_win);
    }
    i++;
	}

  destroy_session(session);
	fflush(logfile);
	delwin(settings_win);
	delwin(visusaliser_win);
	endwin();
	close_logfile();
  fprintf(logfile, "purses exited successfully!\n");
	// exit with success status code
	return 0;
}
