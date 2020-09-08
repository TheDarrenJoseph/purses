#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

void draw_bar(WINDOW* win, int start_x, int height, const char* label){
	int start_y = 38;
	mvwprintw(win, start_y, start_x, label);
	init_pair(1, COLOR_GREEN, COLOR_GREEN);
	wattron(win, COLOR_PAIR(1));
	for (int i=1; i<height; i++) {
		mvwprintw(win, start_y-i, start_x, "A");
	}
	wattroff(win, COLOR_PAIR(1));
}

void draw_visualiser(WINDOW* win) {
	box(win, 0, 0);
	draw_bar(win, 1, 38,  "40Hz ");
	draw_bar(win, 6, 38,  "100Hz");
	draw_bar(win, 11, 38, "300Hz");
	draw_bar(win, 16, 38, "600Hz");
	draw_bar(win, 21, 38, "1kHz ");
	draw_bar(win, 26, 38, "2kHz ");
	draw_bar(win, 31, 38, "4kHz ");
	draw_bar(win, 36, 38, "6kHz ");
	draw_bar(win, 41, 38, "9kHz ");
	draw_bar(win, 46, 38, "12kHz");
	draw_bar(win, 51, 38, "16kHz");
	wrefresh(win);
}
