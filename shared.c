#include <stdio.h>

const char* LOGFILE_NAME = "purses.log";

FILE* logfile = 0;

FILE* get_logfile() {
	if (logfile == 0) {
		logfile = fopen(LOGFILE_NAME, "w");
	}
	return logfile;
}

int close_logfile() {
	if (logfile != 0) {
		//fflush(logfile);
		return fclose(logfile);
	} else {
		return 0;
	}
}
