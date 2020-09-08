#!/bin/sh
gcc -g3 -Wall -lm purses.c pulsehandler.c shared.c processing.c visualiser.c -l ncurses -l pulse -I .
