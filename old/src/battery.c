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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inout.h"

/* Function for getting the current charge level (returns -1 on error). */
int battery_level(char *path)
{
	char *capacity = readfile(path, "capacity");
	int level;

	// Stop if nothing is read
	if (capacity == NULL)
	{
		writelog(1, "Failed to read battery level");
		free(capacity);
		return -1;
	}

	// Convert the char array to an int
	sscanf(capacity, "%d", &level);
	free(capacity);
	writelog(3, "Battery level: '%d'", level);
	return level;
}

/* Find the current status of the battery (returns 'E' on error). */
char battery_status(char *base)
{
	char status;
	char *buff = readfile(base, "status");

	if (!strncmp(buff, "Discharging", 11)) {
		writelog(3, "Battery status: 'Discharging'");
		status = 'v';
	} else if(!strncmp(buff, "Not charging", 12)) {
		writelog(3, "Battery status: 'Not charging'");
		status = ' ';
	} else if(!strncmp(buff, "Full", 4)) {
		writelog(3, "Battery status: 'Full'");
		status = ' ';
	} else if(!strncmp(buff, "Charging", 8)) {
		writelog(3, "Battery status: 'Charging'");
		status = '^';
	} else {
		buff[strlen(buff) - 1] = '\0';
		writelog(2, "Read an unknown status '%s'", buff);
		status = '?';
	}

	free(buff);
	return status;
}

/* Retrive battery information if avaliable. */
char *battery(int n)
{
	char *buff = malloc(8);
	char path[29];
	snprintf(path, 29, "/sys/class/power_supply/BAT%d", n);
	memset(buff, 0, 8);

	// Test if the battery is present
	char *present = readfile(path, "present");
	if (present == NULL)
	{
		// A battery is only present if a '1' was read.
		writelog(3, "No battery present");
		free(present);
		strcpy(buff, "");
		return buff;
	}
	free(present);

	// Default return value if a battery is detected but the
	// 	level or status could not be read.
	strcpy(buff, "| ERROR");

	// Get the current battery level
	int level = battery_level(path);
	if (level == -1)
		return buff;

	// Get the battery status
	char status = battery_status(path);
	if (status == 'E')
		return buff;

	// Format and return the final string.
	snprintf(buff, 8, "| %d%%%c", level, status);
	return buff;
}
