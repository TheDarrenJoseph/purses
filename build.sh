#!/bin/sh
gcc -Wall purses.c pulsehandler.c -l ncurses -l pulse -I .
