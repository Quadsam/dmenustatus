// dmenustatus - a statusbar for dwm's dmenu
// Copyright (C) 2023-2026  Quadsam
// Licensed under GNU Affero General Public License v3.0

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
#include <getopt.h>
#include <poll.h>
#include <alsa/asoundlib.h>
#include <sensors/sensors.h>

/* ========================================================================= */
/* GLOBALS																	 */
/* ========================================================================= */

#define BUFFER_SIZE 128
#define VERSION "0.10.4"

Display *display;
int verbose = 3;
int test_count = 0;
bool running = true;
bool daemonize = false;
snd_mixer_t *mixer_handle = NULL; // Persistent ALSA connection

/* ========================================================================= */
/* HELPER FUNCTIONS															 */
/* ========================================================================= */

// Logging function
//   level    = 0-5 (FATAL, ERROR, WARN, INFO, DEBUG, LOG)
//   fmt, ... = format string (like printf)
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

// Get signal name from signal number
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

// Cleanup when we are told to quit
static
void cleanup(int sig)
{
	writelog(0, "Caught signal '%s'", getsig(sig));
	running = false;
}

/* ========================================================================= */
/* SETUP FUNCTIONS															 */
/* ========================================================================= */

// Open a handle to ALSA
static
void init_mixer()
{
	// Open the mixer once and keep it open
	if (snd_mixer_open(&mixer_handle, 0) < 0) { writelog(1, "Failed to open ALSA mixer"); return; }
	if (snd_mixer_attach(mixer_handle, "default") < 0) { writelog(1, "Failed to attach default card"); return; }
	if (snd_mixer_selem_register(mixer_handle, NULL, NULL) < 0) { writelog(1, "Failed to register mixer elements"); return; }
	if (snd_mixer_load(mixer_handle) < 0) { writelog(1, "Failed to load mixer elements"); return; }
	writelog(3, "ALSA mixer initialized");
}

// Parse command line arguments
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

/* ========================================================================= */
/* MODULES																	 */
/* ========================================================================= */

static
bool get_vol(char *buff, size_t buff_size)
{
	if (!mixer_handle) return false;

	long min, max, vol;
	int switch_state;
	snd_mixer_selem_id_t *sid;

	// Find "Master" element
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, "Master");
	snd_mixer_elem_t *elem = snd_mixer_find_selem(mixer_handle, sid);

	if (!elem) return false;

	// Get Volume Range & Value
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &vol);

	// Check Mute Status (Switch) -> 0 = Muted, 1 = Unmuted
	if (snd_mixer_selem_has_playback_switch(elem))
		snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &switch_state);
	else
		switch_state = 1;

	// Calculate Percentage
	// Avoid division by zero if min == max
	int percentage = 0;
	if (max - min > 0) {
		percentage = (int)(((float)(vol - min) / (max - min)) * 100);
	}

	if (buff && buff_size > 0) {
		size_t len = strlen(buff);
		if (len < buff_size) {
			const char *icon = (switch_state) ? "VOL" : "MUT";
			snprintf(buff + len, buff_size - len, "| %s %d%% ", icon, percentage);
		}
	}
	return true;
}

static
bool get_temp(char *buff, size_t buff_size)
{
	const sensors_chip_name *chip;
	int chip_nr = 0;
	double val = -1.0;
	bool found = false;

	// Iterate through chips to find the CPU temperature
	while ((chip = sensors_get_detected_chips(NULL, &chip_nr))) {
		int feature_nr = 0;
		const sensors_feature *feature;
		while ((feature = sensors_get_features(chip, &feature_nr))) {
			char *label = sensors_get_label(chip, feature);
			if (label && (strstr(label, "CPU Temperature") || strstr(label, "Package"))) {	// Filter for the main temperature
				const sensors_subfeature *sub = sensors_get_subfeature(chip, feature, SENSORS_SUBFEATURE_TEMP_INPUT);
				if (sub && sensors_get_value(chip, sub->number, &val) == 0) {
					found = true;
					free(label);
					break;
				}
			}
			free(label);
		}
		if (found) break;
	}

	if (!found) {
		writelog(1, "Could not find CPU temperature via libsensors");
		return false;
	}

	if (buff != NULL && buff_size > 0) {
		size_t len = strlen(buff);
		if (len + 1 < buff_size)
			snprintf(buff + len, buff_size - len, "| %.0f°C ", val);
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

	// Default status
	char *status = "?";
	file = fopen("/sys/class/power_supply/BAT0/status", "r");
	if (file && fgets(data, sizeof(data), file)) {
		switch (data[0]) {
		case 'D': status = "v"; break; // D(ischarging)
		case 'N': status = "-"; break; // N(ot charging)
		case 'F': status = " "; break; // F(ull)
		case 'C': status = "^"; break; // C(harging)
		}
	}
	fclose(file);

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
	if (!buff || buff_size == 0) return false;
	time_t now = time(NULL);			// Get current time
	struct tm *t = localtime(&now);		// Convert to local time structure
	if (!t) return false;
	strftime(buff, buff_size, " %I:%M:%S %p | %m/%d/%Y ", t);
	return true;
}



/* ========================================================================= */
/* MAIN ENTRY POINT															 */
/* ========================================================================= */

int main(int argc, char **argv)
{
	// Cleanup on these signals
	signal(SIGHUP, cleanup);
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGTERM, cleanup);

	// Read command line arguments
	parse_args(argc, argv);

	if (daemonize) {
		writelog(3, "Daemonizing!");
		if (daemon(0, 1) < 0) {
			writelog(0, "Failed to daemonize process");
			perror("daemon"); exit(EXIT_FAILURE);
		}
	}

	// Open the X display
	display = XOpenDisplay(getenv("DISPLAY"));
	if (!display) {
		writelog(0, "Cannot open X11 display. Is X server running?");
		exit(EXIT_FAILURE);
	}
	Window window = DefaultRootWindow(display);

	init_mixer();	// Initialize ALSA persistent connection

	// Allocate and zero our buffer
	char *buffer = malloc(BUFFER_SIZE);
	if (!buffer) {
		writelog(0, "Memory allocation failed for main buffer");
		XCloseDisplay(display);
		return 1;
	}

	if (sensors_init(NULL) != 0)
		writelog(1, "Failed to initialize libsensors");

	bool enable_temp = get_temp(NULL, 0);
	bool enable_batt = get_batt(NULL, 0);
	bool enable_vol  = get_vol(NULL, 0);

	/* POLL SETUP */
	// Calculate how many file descriptors ALSA needs
	int alsa_count = 0;
	struct pollfd *fds = NULL;
	if (mixer_handle) {
		alsa_count = snd_mixer_poll_descriptors_count(mixer_handle);
		if (alsa_count > 0) {
			fds = calloc (alsa_count, sizeof(struct pollfd));
			snd_mixer_poll_descriptors(mixer_handle, fds, alsa_count);
		}
	}

	writelog(3, "Starting event loop. Temp: %s, Battery: %s, Volume: %s",
				enable_temp ? "Enabled" : "Disabled",
				enable_batt ? "Enabled" : "Disabled",
				enable_vol  ? "Enabled" : "Disabled");

	// Main POLL loop
	while (running) {
		buffer[0] = '\0'; // Clear our buffer

		// Begins the buffer with the current time and date (HH:MM:SS %p | MM/DD/YYYY)
		if (!get_datetime(buffer, BUFFER_SIZE)) {
			writelog(1, "Unable to get current date and time, something is wrong!");
			break;
		}

		// Concatenate the CPU temp (00°C), battery level (00%), and volume level (VOL 00%) to the buffer
		if (enable_temp) get_temp(buffer, BUFFER_SIZE);
		if (enable_batt) get_batt(buffer, BUFFER_SIZE);
		if (enable_vol)  get_vol(buffer, BUFFER_SIZE);

		// Write buffer to X display (and DEBUG log)
		XStoreName(display, window, buffer);
		XSync(display, False);
		writelog(4, "Status update: '%s'", buffer);

		if (test_count > 0 && --test_count == 0)
			running = false;

		if (running) {
			// Wait 1 second OR until ALSA wakes us up
			if (mixer_handle && fds) {
				// If poll returns > 0, we clear the event and restart the loop
				if (poll(fds, alsa_count, 1000) > 0)
					snd_mixer_handle_events(mixer_handle);
			} else {
				// Fallback if ALSA failed: just sleep
				sleep(1);
			}
		}
	}

	// Cleanup
	writelog(3, "Cleaning up resources");
	if (fds) free(fds);
	if (mixer_handle) snd_mixer_close(mixer_handle);
	free(buffer);
	XCloseDisplay(display);
	sensors_cleanup();
	writelog(3, "Exiting");
	return 0;
}
