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

// Draw 10 decibel increments up to 420dB
void draw_y_labels(WINDOW* win) {
	// Account for the boxing of the window
	int start_y = VIS_HEIGHT-2;
	for (int i=0; i<start_y; i+=6) {
		mvwprintw(win, start_y-i, 0, "%ddB", i*10);
	}
}

void update_graph(WINDOW* win, complex_set_t* output_set) {
	complex_wrapper_t* complex_vals = output_set -> complex_numbers;
	draw_bar(win, 5, calculate_height(complex_vals[1]),  "43Hz "); // 1 
	draw_bar(win, 11, calculate_height(complex_vals[3]),  "129Hz"); // 3
	draw_bar(win, 16, calculate_height(complex_vals[7]), "301Hz"); // 7
	draw_bar(win, 21, calculate_height(complex_vals[14]), "600Hz"); // 14
	draw_bar(win, 26, calculate_height(complex_vals[24]), "1kHz "); // 24 
	draw_bar(win, 31, calculate_height(complex_vals[47]), "2kHz "); // 47
	draw_bar(win, 36, calculate_height(complex_vals[94]), "4kHz "); // 94
	draw_bar(win, 41, calculate_height(complex_vals[140]), "6kHz "); // 140
	draw_bar(win, 46, calculate_height(complex_vals[210]), "9kHz "); // 210
	draw_bar(win, 51, calculate_height(complex_vals[280]), "12kHz"); // 280
	draw_bar(win, 57, calculate_height(complex_vals[373]), "16kHz"); // 373
	wrefresh(win);
} 

void draw_visualiser(WINDOW* win, complex_set_t* output_set) {
	werase(win);
	box(win, 0, 0);
	draw_y_labels(win);
	update_graph(win, output_set);
}
