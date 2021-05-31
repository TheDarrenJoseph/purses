#include <ncurses.h>
#include <pulseaudio/pulsehandler.h>
#include <settings.h>

// Gets a list of PulseAudio Sinks
// Returns 0 if the listing was successful, 1 in the event of an error
int get_sinks(pa_device_t* device_list, int* count) {
  int sink_list_stat = get_sinklist(device_list, count);
	return (sink_list_stat != 0) ? 1 : 0;
}

pa_device_t get_main_device() {
	int count = 0;
  pa_device_t* sink_list = malloc(sizeof(pa_device_t) * DEVICE_MAX);
  get_sinks(sink_list, &count);
  print_devicelist(sink_list, DEVICE_MAX);
  pa_device_t* chosen_device = malloc(sizeof(pa_device_t));
  // Copy the device we want
  chosen_device = &sink_list[0];
  free(sink_list);
  return (*chosen_device);
}

// Gets a single character of input from the provided window
// Returning an integer of 0 if nothing is matched
// 1 for quit
// 2 for settings menu
// 3 to decrement the device index
// 4 to increment the device index
int handle_setting_input(WINDOW* window) {
  keypad(window, true);
	int keypress = wgetch(window);
  char input = (char) keypress;

  switch(keypress) {
    case ERR:
      return 0;
    case KEY_UP:
      return 3;
    case KEY_DOWN:
      return 4;
  }

  switch(input) {
    case 'q':
      return 1;
    case '\n':
    case '\r':
      return 2;
    case 'u':
      return 3;
    case 'd':
      return 4;
  } 

  return 0;
}

void draw_options(WINDOW* settings_window, int count, pa_device_t* sink_list, int* chosen_device_index) {
  for (int i=0; i<count; i++) {
    bool selected_device = (i == *chosen_device_index);
    if (selected_device) {
      wattron(settings_window, A_REVERSE);
    }
    mvwprintw(settings_window, 1+i, 2, "%d. %s", i+1, sink_list[i].description);
    if (selected_device) {
      wattroff(settings_window, A_REVERSE);
    }
  }
}

// Shows a window for choosing the current device in use
// Returns a pa_device_t repesenting the active or new device choice
pa_device_t show_device_choice_window(WINDOW* settings_window, int* device_index) {
	int count = 0;
  pa_device_t* sink_list = malloc(sizeof(pa_device_t) * DEVICE_MAX);
	get_sinks(sink_list, &count);

  int chosen_device_index = *device_index;
  while (true) {
    werase(settings_window);
    draw_options(settings_window, count, sink_list, &chosen_device_index);
    box(settings_window, 0, 0);

    char* banner = "===Device (Pulseaudio Sink) Selection===";
    mvwprintw(settings_window, 0, 5, banner);
    mvwprintw(settings_window, SETTINGS_HEIGHT-1, 1, "%d q - Close, Enter - Select", chosen_device_index);
  
		int command_code = handle_setting_input(settings_window);
    int max_choice = count > 0 ? count-1 : 0;
		if (command_code == 1) {
      break;
    }
		if (command_code == 2) {
      *device_index = chosen_device_index;
      break;
    }
		if (command_code == 3 && chosen_device_index > 0) {
      chosen_device_index--;
    }
		if (command_code == 4 && chosen_device_index < max_choice) {
      chosen_device_index++;
    }
  }
    
  return sink_list[chosen_device_index];

  free(sink_list);
}



