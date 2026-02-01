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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "dmenustatus.h"
#include "utils.h"

int writelog(int v, char *fmt, ...);
void parse_args(int argc, char **argv);
void handle_signal(int sig);
char *readfile(char *base, char *file);


/* Parse arguments passed. */
void parse_args(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, ":fhHqt:vV")) != -1)
		switch (c)
		{
			case 'f':
				forked = 1;
				break;
			case 'h':
			case 'H':
				printf("Usage: dmenustatus [OPTION]\n\n");
				printf("Options:\n");
				printf("  -q,      Set verbosity to zero (no output).\n");
				printf("  -t <n>,  Run the main program loop 'n' times.\n");
				printf("  -v,      Increase verbosity by one (use multiple times to increase verbosity).\n");
				printf("  -V,      Display current version.\n");
				exit(EXIT_SUCCESS);
			case 'V':
				printf("%s v%s\n", "dmenustatus", VERSION);
				exit(EXIT_SUCCESS);                
			case 'q':
				verbose = 0;
				break;
			case 'v':
				verbose++;
				writelog(5, "Loglevel set to: %d", verbose);
				break;
			case 't':
				testtimes = atoi(optarg);
				if (testtimes == 0)
					testtimes++;
				testing = 1;
				writelog(3, "Running in test mode");
				break;
			case '?':
				writelog(0, "Illegal option -- '-%c'", optopt);
				exit(EXIT_FAILURE);
			case ':':
				writelog(0, "Missing argument for -- '-%c'", optopt);
				exit(EXIT_FAILURE);
	}
}

/* Write a message to stdout */
int writelog(int v, char *fmt, ...)
{
	int n = 0;
	size_t size = 0;
	char *p = NULL;
	char *prefix = NULL;
    char log_time[23];
    time_t timep;
    struct tm *tm;
	va_list ap;

	if ((v != 5) && (v > verbose))
		return 1;

	va_start(ap, fmt);
	n = vsnprintf(p, size, fmt, ap);
	va_end(ap);

	if (n < 0)
		return 0;

	size = (size_t) n + 1;
	p = malloc(size);

	if (p == NULL)
		return 0;

	va_start(ap, fmt);
	n = vsnprintf(p, size, fmt, ap);
	va_end(ap);

	if (n < 0)
	{
		free(p);
		return 0;
	}
	switch (v)
	{
		case 0:
			prefix = " F:";
			break;
		case 1:
			prefix = " E:";
			break;
		case 2:
			prefix = " W:";
			break;
		case 3:
			prefix = " I:";
			break;
		case 4:
			prefix = " D:";
			break;
		case 5:
			prefix = "";
			break;
	}

	timep = time(NULL);
    tm = localtime(&timep);
    strftime(log_time, sizeof(log_time)-1, "%x %X", tm);
	fprintf(stderr, "[ %s ]%s %s\n", log_time, prefix, p);
	free(p);
	return 1;
}

/* Used to stop the main loop when a signal is recived */
void handle_signal(int sig)
{
    writelog(2, "Caught signal %d", sig);
    running = 0;
}

/* Read from a file */
char *readfile(char *base, char *file)
{
	char *data = malloc(513);
	char path[64];
	FILE *fd;

	memset(data, 0, 513);
	snprintf(path, 64, "%s/%s", base, file);
	writelog(4, "Opening file: '%s'", path);
	fd = fopen(path, "r");
	if (fd == NULL)
	{
		writelog(2, "Failed to open file '%s'", path);
		data[0] = '\0';
		return data;
	}

	if (fgets(data, 512, fd) == NULL)
	{
		writelog(2, "File contains no data '%s'", path);
		data[0] = '\0';
	}
	fclose(fd);
	return data;
}
