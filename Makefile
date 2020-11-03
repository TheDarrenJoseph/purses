compile:
	gcc -g3 -Wall -lm purses.c pulsehandler.c shared.c processing.c visualiser.c -l ncurses -l pulse -I. -o purses.out

test:
	gcc -g3 -Wall -lm tests.c pulsehandler.c shared.c processing.c -l pulse -I . -o tests.out
