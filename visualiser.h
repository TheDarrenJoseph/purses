#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

#include <shared.h>

#define VIS_HEIGHT 45
#define VIS_WIDTH 120


void draw_bar(WINDOW* win, int start_x, int height, const char* label);

void draw_visualiser(WINDOW* win, complex_set_t* output_set);
