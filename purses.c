#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <pulsehandler.h>
#include <shared.h>
#include <processing.h>

int println (char* format, va_list vlist) {
	char* output = strcat(format, "\n");
	return vprintf(output, vlist);
}

long seek_file_size(FILE* file) {
	FILE* logfile = get_logfile();

	fprintf(logfile, "Capturing initial file pos... \n");

	fpos_t original_file_pos;
	fgetpos(file, &original_file_pos);
	
	fprintf(logfile, "Seeking end of file... \n");
	
	//Set position indicator to 0 from the end of the FILE*
	fseek(file, 0, SEEK_END);
	// Once seeked, get the position
	long file_size = ftell(file);
	// Reset to original pos
	
	fsetpos(file, &original_file_pos);
	return file_size;
}

void write_to_file(record_stream_data_t* stream_read_data, char* filename) {
	FILE* logfile = get_logfile();
	FILE* outfile = fopen(filename, "w");
	
	int16_t* record_samples = stream_read_data -> data;
	int sample_count = stream_read_data -> data_size;
	if (sample_count > 0) {
		fprintf(logfile, "Writing record stream of %d samples to file: %s...\n", sample_count, filename);
		fwrite(record_samples, sizeof(int16_t), sample_count, outfile);
	} else {
		printf("Given record_stream_data_t has a data_size of 0!\n");
	}	
	
	fclose(outfile);

}

// Fills in the record_stream_data_t from the file named
// Reads up to data_size (count) samples
void read_from_file(record_stream_data_t* stream_read_data, char* filename) {
	FILE* logfile = get_logfile();

	int count = stream_read_data -> data_size;
	fprintf(logfile, "Reading record stream of %d samples from file: %s...\n", count, filename);

	FILE* outfile = fopen(filename, "r");
	long file_size = seek_file_size(outfile);
	fprintf(logfile, "File is %ld bytes long.\n", file_size);

	int16_t* record_data = stream_read_data -> data;
	void* read_buff = record_data;
	fread(read_buff, sizeof(int16_t), count, outfile);
	for (int i=0; i<count; i++) {
		int16_t sample_data = record_data[i];
		fprintf(logfile, "index: %ld, data: %d\n", i , (signed int) sample_data);
	}
	
	fclose(outfile);
}

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
	int* buffer_size = 0;
	record_device(main_device, &stream_read_data);
	printw("Recording complete...\n");
	
	write_to_file(stream_read_data, "white.bin");
	
	record_stream_data_t* file_read_data = 0;
	init_record_data(&file_read_data);
	read_from_file(file_read_data, "white.bin");
	
	// FFT The PCM Data
	//ct_fft(stream_read_data);
	
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
