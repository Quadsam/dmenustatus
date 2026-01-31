#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define BUFFER_SIZE 128 * sizeof(char)

Display *display;

static
bool get_datetime(char *buff)
{
	size_t success;
	time_t now = time(NULL);			// Get current time
	struct tm *t = localtime(&now);		// Convert to local time structure
	char formatted[64];

	success = strftime(formatted, sizeof(formatted), " %I:%M:%S %p | %m/%d/%Y ", t);
	if (success == 0)
		return false;

	buff = strcpy(buff, formatted);

	return true;
}

static
bool get_temp(char *buff)
{
	const char *path = "/sys/devices/virtual/thermal/thermal_zone0/temp";

	FILE *file = fopen(path, "r");
	if (file == NULL)
		return false; // File doesnt exists

	char *data = malloc(513);
	if (fgets(data, 512, file) == NULL) {
		free(data);
		fclose(file);
		return false; // File contains no data
	}
	fclose(file);

	float temp = atof(data) / 1000;
	char formatted[64];
	sprintf(formatted, "| %02.0f°C ", temp);
	free(data);

	if (buff != NULL)
		buff = strcat(buff, formatted);


	return true;
}

static
bool get_batt(char *buff)
{
	bool have_battery = true; // Use test to define

	if (!have_battery)
		return false;

	// Get batt logic

	char *formatted_batt = "| 100%- ";
	if (buff != NULL)
		buff = strcat(buff, formatted_batt);

	return true;
}

int main(void)
{
	// Open the X display
	if (!(display = XOpenDisplay(NULL)))
	{
		printf("Error: Cannot open display.\n");
		exit(EXIT_FAILURE);
	}

	// Allocate and zero our buffer
	char *buffer = malloc(BUFFER_SIZE);

	bool running = true;
	bool enable_temp = get_temp(NULL);
	bool enable_batt = get_batt(NULL);
	while (running) {
		memset(buffer, '\0', BUFFER_SIZE);

		// Begins the buffer with the current time and date (HH:MM:SS %p | MM/DD/YYYY)
		if (!get_datetime(buffer)) {
			printf("Error: Unable to get current date and time, something is wrong.\n");
			free(buffer);
			exit(EXIT_FAILURE);
		}

		// Concatenate the CPU temp to the buffer (00°C)
		if (enable_temp)
			if (!get_temp(buffer))
				printf("Info: Unable to get current temp.\n");

		// Concatenate the battery level to the buffer (00°C)
		if (enable_batt)
			if (!get_batt(buffer))
				printf("Info: Unable to get battery level.\n");

		// Write buffer to X display
		XStoreName(display, DefaultRootWindow(display), buffer);
		XSync(display, False);

		printf("'%s'\n", buffer);

		sleep(1);
	}

	free(buffer);

	return 0;
}