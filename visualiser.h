#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <ncurses.h>

void draw_bar(WINDOW* win, int start_x, int height);

void draw_visualiser(WINDOW* win);
