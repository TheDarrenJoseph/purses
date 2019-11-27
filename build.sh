#!/bin/sh
gcc -g3 -Wall purses.c pulsehandler.c shared.c -l ncurses -l pulse -I .
