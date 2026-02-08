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

#define _DEFAULT_SOURCE // Required for daemon() feature test macros

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <fontconfig/fontconfig.h>
#include <getopt.h>

#define BUFFER_SIZE 128
#define VERSION "0.9.6-alpha"

int verbose = 3;
int test_count = 0;
bool daemonize = false;
bool use_nerd = false;
bool running = true;
Display *display;

static
const char *getsig(int sig)
{
	switch (sig) {
	case 1:  return "SIGHUP";
	case 2:  return "SIGINT";
	case 3:  return "SIGQUIT";
	case 15: return "SIGTERM";
	default: return "UNKNOWN";
	}
}

static
void writelog(int level, const char *fmt, ...)
{
	if (level > verbose) return;

	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char time_str[32];
	strftime(time_str, sizeof(time_str), "%x %X", t);

	const char *prefix;
	switch (level) {
	case 0: prefix = "FATAL"; break;
	case 1: prefix = "ERROR"; break;
	case 2: prefix = "WARN "; break;
	case 3: prefix = "INFO "; break;
	case 4: prefix = "DEBUG"; break;
	default: prefix = "LOG  "; break;
	}

	fprintf(stderr, "[ %s ] %s: ", time_str, prefix);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");
}

static
void cleanup(int sig)
{
	running = false;
	writelog(0, "Caught signal '%s'", getsig(sig));
	return;
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
			writelog(3, "Increasing verbosity to: %d", verbose);
			break;
		case 'V':
			printf("%s v%s\n", argv[0], VERSION);
			exit(EXIT_SUCCESS);
			break;
		case '?':
			writelog(0, "Illegal option -- '-%c'", optopt);
			exit(EXIT_FAILURE);
			break;
		case ':':
			writelog(0, "Missing argument for -- '-%c'", optopt);
			exit(EXIT_FAILURE);
			break;
		}
	return;
}

static
bool has_nerd_font() {
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
				writelog(4, "Found Nerd Fonts\n");
				break;
			} else {
				writelog(4, "Couldn't find Nerd Fonts\n");
			}
		}
	}

	FcFontSetDestroy(fs);
	return found;
}

static
bool get_temp(char *buff, size_t buff_size)
{
	FILE *file = fopen("/sys/class/hwmon/hwmon4/temp1_input", "r");
	if (!file) {
		file = fopen("/sys/devices/virtual/thermal/thermal_zone0/temp", "r");
		if (!file)
			return false;
	}

	char data[16];
	if (fgets(data, sizeof(data), file) == NULL) {
		writelog(1, "temperature file has no data!");
		fclose(file);
		return false;
	}
	fclose(file);

	// Read the temperature as a floating point number
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
	if (!file) {
		writelog(1, "/sys/class/power_supply/BAT0/capacity is missing!");
		return false;
	}

	char data[16];
	int level = 0;
	if (fgets(data, sizeof(data), file))
		level = atoi(data);

	fclose(file);

	char *status = "?";
	// Set status to a question mark by default (not changed unless recognized)
	if (use_nerd && strcmp(status, "?") == 0) status = "󰂃"; // Upgrade to icon if needed
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

int main(int argc, char **argv)
{
	// Cleanup on these signals
	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);

	parse_args(argc, argv);

	if (daemonize) {
		writelog(3, "Daemonizing!");
		if (daemon(0, 1) < 0) {
			writelog(0, "Failed to daemonize process");
			perror("daemon");
			exit(EXIT_FAILURE);
		}
	}

	// Open the X display
	if (!(display = XOpenDisplay(NULL)))
	{
		writelog(0, "Cannot open X11 display. Is X server running?");
		exit(EXIT_FAILURE);
	}

	use_nerd = has_nerd_font();

	// Allocate and zero our buffer
	char *buffer = malloc(BUFFER_SIZE);
	if (!buffer) {
		writelog(0, "Memory allocation failed for main buffer");
		XCloseDisplay(display);
		return 1;
	}

	bool enable_temp = get_temp(NULL, 0);
	bool enable_batt = get_batt(NULL, 0);

	writelog(3, "Starting main loop. Temp: %s, Battery: %s", 
				enable_temp ? "Enabled" : "Disabled", 
				enable_batt ? "Enabled" : "Disabled");

	while (running) {
		buffer[0] = '\0'; // Clear our buffer

		// Begins the buffer with the current time and date (HH:MM:SS %p | MM/DD/YYYY)
		if (!get_datetime(buffer, BUFFER_SIZE)) {
			writelog(1, "Unable to get current date and time, something is wrong!");
			break;
		}

		// Concatenate the CPU temp to the buffer (00°C)
		if (enable_temp) get_temp(buffer, BUFFER_SIZE);

		// Concatenate the battery level to the buffer (00°C)
		if (enable_batt) get_batt(buffer, BUFFER_SIZE);

		// Write buffer to X display
		XStoreName(display, DefaultRootWindow(display), buffer);
		XSync(display, False);

		writelog(4, "Status update: '%s'", buffer);

		if (test_count > 0) {
			test_count--;
			if (test_count == 0)
				running = false;
		}

		if (running) sleep(1);
	}

	writelog(3, "Cleaning up resources");
	free(buffer);
	XCloseDisplay(display);
	writelog(3, "Exiting");

	return 0;
}