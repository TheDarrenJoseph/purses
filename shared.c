#include <stdio.h>
#include <shared.h>

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

void fprint_data(FILE* file, complex_set_t* samples) {
	int sample_rate = samples -> sample_rate;
	int data_size = samples -> data_size;
	// Frequency resolution = Sampling Freq / Sample Count
	int freq_resolution = sample_rate / data_size;	
	fprintf(file, "Frequency Resolution: %dHz\n", freq_resolution);
	
	double sum = 0.0;
	for (int i=0; i<data_size; i++) {
		int frequency = freq_resolution * i;
		struct complex_wrapper wrapper = samples -> complex_numbers[i];
		
		complex double complex_val = wrapper.complex_number;
		
		double realval = creal(complex_val);
		sum += realval;
		double imval = cimag(complex_val);
		double mag = wrapper.magnitude;
		fprintf(file, "(%d - %dHz) - Real: %.2f, Imaginary: %+.2fi, Magnitude: %.2f\n", i, frequency, realval, imval, mag);
	}
}

void print_data(complex_set_t* samples) {
	fprint_data(stdout, samples);
}

void fprintln (char* format, va_list vlist) {
	int current_len = strlen(format);
	
	// Add space for the newline char
	char expanded[current_len+1];
	// Copy and append the newline
	strcat(expanded, format);
	strcat(expanded, "\n");
	
	if (vlist != NULL) {
		vprintf(expanded, vlist);
	} else {
		printf(expanded);
	}
}

void printlncol( char* ansi_code, char* format) {
	printf("%s%s",ESC,ansi_code);
	fprintln(format, NULL);
	printf("%s%s",ESC,ANSI_DEFAULT);
}

long seek_file_size(FILE* file) {
	FILE* logfile = get_logfile();

	fprintf(logfile, "Capturing initial file pos... \n");

	fpos_t original_file_pos;
	fgetpos(file, &original_file_pos);
	
	fprintf(logfile, "Seeking end of file... \n");
	
	//Set position indicator to 0 from the end of the FILE*
	fseek(file, 0, SEEK_END);
	// Once seeked, get the position
	long file_size = ftell(file);
	// Reset to original pos
	
	fsetpos(file, &original_file_pos);
	return file_size;
}

void write_to_file(record_stream_data_t* stream_read_data, char* filename) {
	FILE* logfile = get_logfile();
	FILE* outfile = fopen(filename, "w");
	
	int16_t* record_samples = stream_read_data -> data;
	int sample_count = stream_read_data -> data_size;
	if (sample_count > 0) {
		fprintf(logfile, "Writing record stream of %d samples to file: %s...\n", sample_count, filename);
		fwrite(record_samples, sizeof(int16_t), sample_count, outfile);
	} else {
		printf("Given record_stream_data_t has a data_size of 0!\n");
	}	
	
	fclose(outfile);

}

// Fills in the record_stream_data_t from the file named
// Reads up to data_size (count) samples
void read_from_file(record_stream_data_t* stream_read_data, char* filename) {
	FILE* logfile = get_logfile();

	int count = stream_read_data -> data_size;
	fprintf(logfile, "Reading record stream of %d samples from file: %s...\n", count, filename);

	FILE* outfile = fopen(filename, "r");
	long file_size = seek_file_size(outfile);
	fprintf(logfile, "File is %ld bytes long.\n", file_size);

	int16_t* record_data = stream_read_data -> data;
	void* read_buff = record_data;
	fread(read_buff, sizeof(int16_t), count, outfile);
	for (int i=0; i<count; i++) {
		int16_t sample_data = record_data[i];
		fprintf(logfile, "index: %d, data: %d\n", i , (signed int) sample_data);
	}
	
	fclose(outfile);
}
