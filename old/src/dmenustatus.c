// dmenustatus - a statusbar for dwm's dmenu
// Copyright (C) 2023  Quadsam
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "battery.h"
#include "inout.h"
#include "utils.h"

int verbose = 3;
int running = 1;
int testing = 0;
int testtimes = 0;
int forked = 0;

int main(int argc, char **argv)
{
	int step = 1;

	// Parse any arguments passed.
	parse_args(argc, argv);

	if(!forked)
	{
		char *arg[argc + 2];
		arg[0] = argv[0];
		arg[1] = "-f";
		for(int i = 1; i != argc; i++)
			arg[i + 1] = argv[i];

		arg[argc + 1] = NULL;
		spawn(argv[0], arg);
		exit(EXIT_SUCCESS);
	}

	// Try to make a connection to the X server.
	if (!open_display())
		return 1;

	// Once a connection has been made, allocate our buffers.
	const int status_sz = 512;
	char *status = malloc(status_sz);
	char *datetime_buff = NULL;
	char *cputemp_buff = NULL;
	char *battery_buff = NULL;

	// Setup the signal handler.
	signal(SIGHUP, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGQUIT, handle_signal);
	signal(SIGTERM, handle_signal);

	// Log the current PID.
	writelog(3, "PID: %d", getpid());

	// Main loop
	while(running)
	{
		// Fill the status buffer with '\0'.
		memset(status, 0, status_sz);

		// Get the current date, time, temp, and battery status.
		datetime_buff = datetime();
		cputemp_buff = cputemp(9);
		battery_buff = battery(0);

		// Write the data recived into the status buffer.
		strcpy(status, datetime_buff);
		strcat(status, cputemp_buff);
		strcat(status, battery_buff);

		// Free the buffers allocated
		free(datetime_buff);
		free(cputemp_buff);
		free(battery_buff);

		// Log & set the status
		writelog(4, "Setting status: '%s'", status);
		setstatus(status);

		// End the loop if we are only in testing mode.
		if (testing && step == testtimes)
		{
			running = 0;
			break;
		}
		else if (testing)
			step++;

		sleep(1);
	}

	// Cleanup the buffers and close the display.
	writelog(4, "Cleaning up...");
	writelog(4, "Freeing buffers");
	memset(status, 0, status_sz);
	free(status);
	writelog(4, "Closing display");
	XCloseDisplay(display);
	writelog(3, "Quitting");
	return 0;
}
