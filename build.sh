#!/bin/sh
gcc -g3 -Wall -lm purses.c pulsehandler.c shared.c processing.c -l ncurses -l pulse -I .
