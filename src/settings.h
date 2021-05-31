#define SETTINGS_HEIGHT 15
#define SETTINGS_WIDTH 80

int get_sinks(pa_device_t* device_list, int* count);
pa_device_t get_main_device();
pa_device_t show_device_choice_window(WINDOW* settings_window, int* device_index);
