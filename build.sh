#!/bin/sh
gcc -Wall purses.c pulsehandler.c shared.c -l ncurses -l pulse -I .
