// dmenustatus - a statusbar for dwm's dmenu
// Copyright (C) 2023-2026  Quadsam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <fontconfig/fontconfig.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define VERSION "0.9.3-alpha"
#define BUFFER_SIZE 128

Display *display;

int verbose = 0;
int test_count = 0;
bool running = true;
bool use_nerd = false;
bool daemonize = false;

static bool has_nerd_font() {
    bool found = false;
    FcConfig* config = FcInitLoadConfigAndFonts();
    
    // We create a "pattern" to search for Nerd Fonts
    FcPattern* pat = FcPatternCreate();
    FcObjectSet* os = FcObjectSetBuild(FC_FAMILY, NULL);
    FcFontSet* fs = FcFontList(config, pat, os);
    FcConfigDestroy(config);
    FcPatternDestroy(pat);
    FcObjectSetDestroy(os);

    for (int i = 0; fs && i < fs->nfont; i++) {
        FcChar8* family;
        if (FcPatternGetString(fs->fonts[i], FC_FAMILY, 0, &family) == FcResultMatch) {
            if (strstr((char*)family, "Nerd Font")) {
                found = true;
                break;
            }
        }
    }

    FcFontSetDestroy(fs);
    return found;
}

static
bool get_datetime(char *buff, size_t buff_size)
{
	if (!buff || buff_size == 0)
		return false;

	time_t now = time(NULL);			// Get current time
	struct tm *t = localtime(&now);		// Convert to local time structure
	if (!t)
		return false;

	strftime(buff, buff_size, " %I:%M:%S %p | %m/%d/%Y ", t);

	return true;
}


static
bool get_temp(char *buff, size_t buff_size)
{
	FILE *file = fopen("/sys/devices/virtual/thermal/thermal_zone0/temp", "r");
	if (!file)
		return false; // File doesnt exist

	char data[16];
	if (fgets(data, sizeof(data), file) == NULL) {
		fclose(file);
		return false; // File has no data
	}
	fclose(file);

	float temp = atof(data) / 1000.0f;

	if (buff != NULL && buff_size > 0) {
		size_t len = strlen(buff);

		// Make sure there is space here
		if (len + 1 < buff_size)
			snprintf(buff + len, buff_size - len, "| %02.0f°C ", temp);
	}

	return true;
}

static
bool get_batt(char *buff, size_t buff_size)
{
	struct stat s;
	if (stat("/sys/class/power_supply/BAT0/present", &s) == -1)
		return false; // No battery installed


	FILE *file = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!file)
		return false;

	char data[16];
	int level = 0;
	if (fgets(data, sizeof(data), file))
		level = atoi(data);

	fclose(file);

	char *status = "󰂃"; // Set status to a question mark by default (not changed unless recognized)
	file = fopen("/sys/class/power_supply/BAT0/status", "r");
	if (file) {
		if (fgets(data, sizeof(data), file)) {
			if (use_nerd) {
				switch (data[0]) {
					case 'D': status = "󰂍"; break; // D(ischarging)
					case 'N': status = "󰂄"; break; // N(ot charging)
					case 'F': status = "󰂄"; break; // F(ull)
					case 'C': status = "󰂐"; break; // C(harging)
				}
			} else {
				switch (data[0]) {
					case 'D': status = "v"; break; // D(ischarging)
					case 'N': status = "-"; break; // N(ot charging)
					case 'F': status = " "; break; // F(ull)
					case 'C': status = "^"; break; // C(harging)
				}
			}
		}
		fclose(file);
	}

	if (buff != NULL && buff_size > 0) {
		size_t len = strlen(buff);
		if (len < buff_size)
			snprintf(buff + len, buff_size - len, "| %d%%%s ", level, status);
	}

	return true;
}

static
void parse_args(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, ":dhqt:vV")) != -1)
		switch (c)
		{
		case 'd':
			daemonize = true;
			break;
		case 'h':
			printf("Usage %s [OPTION]\n\n", argv[0]);
			printf("Options:\n");
			printf("  -d,      Run as a daemon.\n");
			printf("  -h,      Display this help.\n");
			printf("  -q,      Decrease verbosity.\n");
			printf("  -t <n>,  Run main loop 'n' times.\n");
			printf("  -v,      Increase the verbosity.\n");
			printf("  -V,      Display program version.\n");
			exit(EXIT_SUCCESS);
			break;
		case 'q':
			if (verbose != 0) verbose--;
			break;
		case 't':
			test_count = atoi(optarg);
			if (test_count <= 0) test_count = 1;
			break;
		case 'v':
			verbose++;
			printf("Increasing verbosity\n");
			break;
		case 'V':
			printf("%s v%s\n", argv[0], VERSION);
			exit(EXIT_SUCCESS);
			break;
		case '?':
			printf("Illegal option -- '-%c'\n", optopt);
			exit(EXIT_FAILURE);
			break;
		case ':':
			printf("Missing argument for -- '-%c'\n", optopt);
			exit(EXIT_FAILURE);
			break;
		}
	return;
}

static
void cleanup(int sig) {
	running = false;
	printf("Caught signal %d\n", sig);
	return;
}


/*
	TODO: Rewrite logging system
	TODO: - Fatal
	TODO: - Error
	TODO: - Warning
	TODO: - Info
	TODO: - Debug
	TODO: Rewrite daemonizing
	TODO: - setsid()
	TODO: - execvp()
*/

int main(int argc, char **argv)
{
	// Cleanup on these signals
	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);

	parse_args(argc, argv);

	// Open the X display
	if (!(display = XOpenDisplay(NULL)))
	{
		printf("Error: Cannot open display.\n");
		exit(EXIT_FAILURE);
	}
	use_nerd = has_nerd_font();

	// Allocate and zero our buffer
	char *buffer = malloc(BUFFER_SIZE);
	if (!buffer) return 1;

	bool enable_temp = get_temp(NULL, 0);
	bool enable_batt = get_batt(NULL, 0);
	while (running) {
		buffer[0] = '\0'; // Clear our buffer

		// Begins the buffer with the current time and date (HH:MM:SS %p | MM/DD/YYYY)
		if (!get_datetime(buffer, BUFFER_SIZE)) {
			fprintf(stderr, "Error: Unable to get current date and time, something is wrong.\n");
			break;
		}

		// Concatenate the CPU temp to the buffer (00°C)
		if (enable_temp) get_temp(buffer, BUFFER_SIZE);

		// Concatenate the battery level to the buffer (00°C)
		if (enable_batt) get_batt(buffer, BUFFER_SIZE);

		// Write buffer to X display
		XStoreName(display, DefaultRootWindow(display), buffer);
		XSync(display, False);

		if (verbose > 0) printf("'%s'\n", buffer);

		if (test_count > 0) {
			test_count--;
			if (test_count == 0)
				running = false;
		}

		if (running) sleep(1);
	}

	free(buffer);
	XCloseDisplay(display);

	return 0;
}