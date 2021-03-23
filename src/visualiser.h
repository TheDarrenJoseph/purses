#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <shared.h>

#define VIS_HEIGHT 25
#define VIS_WIDTH 120
#define VIS_BARS 11

void draw_bar(WINDOW* win, int start_x, int height, int width, const char* label);

void draw_visualiser(WINDOW* win, complex_set_t* output_set, struct timeval time_taken);
