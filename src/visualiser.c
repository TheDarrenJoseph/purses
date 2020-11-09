#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>
#include <visualiser.h>

int calculate_height(complex_wrapper_t wrapper) {
	int decibels = wrapper.decibels;
	if (decibels > 0) {
		return wrapper.decibels/10;
	}
	return 0;
}

// bin_frequency - the frequency in Hertz per sample bin
// index - the index of the bin in question
// Returns a descriptive string of the frequency multiplied by the index i.e "43Hz" or "16kHz"
char* label_frequency(int bin_frequency, int bin_index) {
	int frequency = bin_frequency * bin_index;
	char* (output) = malloc(sizeof(char) * 6); // 6 chars max
	// If greater than 1000 use the kilo-suffix
	if (frequency > 1000) {
		sprintf(output, "%dkHz", frequency / 1000);
	} else {
		sprintf(output, "%dHz", frequency);
	}
	return output;
}

void draw_bar(WINDOW* win, int start_x, int height, const char* label){
	// Account for the boxing of the window
	int start_y = VIS_HEIGHT-2;
	mvwprintw(win, start_y, start_x, label);
	init_pair(1, COLOR_GREEN, COLOR_GREEN);
	wattron(win, COLOR_PAIR(1));
	for (int i=1; i<height; i++) {
		mvwprintw(win, start_y-i, start_x, "A");
	}
	wattroff(win, COLOR_PAIR(1));
}

// Draw 10 decibel increments
void draw_y_labels(WINDOW* win) {
	// Account for the boxing of the window
	int start_y = VIS_HEIGHT-2;
	for (int i=0; i<start_y; i+=6) {
		mvwprintw(win, start_y-i, 0, "%ddB", i*10);
	}
}

void update_graph(WINDOW* win, complex_set_t* output_set) {
	FILE* logfile = get_logfile();

	// sF/sN (Sample Frequency/Sample Count) = bF (Hertz per bin)
	// 44100/1024 = 43.06

	complex_wrapper_t* complex_vals = output_set -> complex_numbers;
	int data_size = output_set -> data_size;
	int frequency = output_set -> frequency;
	// divide by 2 to get the Nyquist freuency divided by the sample count
	int bin_frequency = (data_size > 0) ? frequency / data_size : 0;
  // Divide the total sample count by 11 bars
	int bin_increment =  (data_size > 0) ? data_size / 11 : 0;
	fprintf(logfile, "%d samples.\n", data_size);
	fprintf(logfile, "%dHz frequency per bin.\n", bin_frequency);
	fprintf(logfile, "%d bin index increment.\n", bin_increment);
	// From 5 to avoid window border, up to 5 + 12 bars
	for (int i=1; i < 12; i++) {
		int bin_index = i*bin_increment;
		char* label = label_frequency(bin_frequency, bin_index);
		fprintf(logfile, "%d bin index == %s\n" , bin_index, label);
		draw_bar(win, i*sizeof(label), calculate_height(complex_vals[bin_index]), label);
	}
	wrefresh(win);
}

void draw_visualiser(WINDOW* win, complex_set_t* output_set) {
	werase(win);
	box(win, 0, 0);
	draw_y_labels(win);
	char* banner = "===PulseAudio ncurses Visualiser===";
	// find the midpoint for our banner
	int target_x = (VIS_WIDTH/2) - sizeof(banner);
	mvwprintw(win, 0, target_x, banner);
	mvwprintw(win, VIS_HEIGHT-1, target_x, "%dSamples@%dHz", output_set -> data_size, output_set -> sample_rate);
	update_graph(win, output_set);
}
