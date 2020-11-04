.PHONY: test
compile:
	gcc -g3 -Wall -lm src/*.c -l ncurses -l pulse -I. -o purses.out

test:
	gcc -g3 -Wall -lm test/tests.c src/pulsehandler.c src/shared.c src/processing.c -l pulse -I . -o tests.out
