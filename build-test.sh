#!/bin/sh
gcc -g3 -Wall -lm tests.c pulsehandler.c shared.c processing.c -l pulse -I . -o tests.out
