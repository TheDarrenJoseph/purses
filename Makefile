.PHONY: test

all: compile test

compile:
	gcc -g3 -Wall -lm src/*.c -lm src/pulseaudio/*.c -l ncurses -l pulse -I src -o purses.out

test:
	gcc -g3 -Wall -lm test/tests.c src/pulsehandler.c src/shared.c src/processing.c -l pulse -I src -o tests.out
