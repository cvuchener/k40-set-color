/*
 * Copyright 2015 Clément Vuchener
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

extern "C" {
#include <unistd.h>
#include <getopt.h>
#include <libusb.h>
}


#define CORSAIR_ID	0x1b1c
#define CORSAIR_K40_ID	0x1b0e

#define REQUEST_SET_COLOR	51

static const char *usage =
	"Usage: %s [options] red green blue\n"
	"Options are:\n"
	" -p<num>,--profile <num>	Set color for profile <num> only.\n"
	" -h,--help			Print this help message.\n"
	;

int main (int argc, char *argv[])
{
	enum {
		ProfileOpt = 256,
		HelpOpt,
	};
	struct option longopts[] = {
		{ "profile", required_argument, nullptr, ProfileOpt },
		{ "help", no_argument, nullptr, HelpOpt },
		{ nullptr, 0, nullptr, 0 }
	};
	int target = 0;
	int color[3];

	int opt;
	while (-1 != (opt = getopt_long (argc, argv, "p:h", longopts, nullptr))) {
		switch (opt) {
		case 'p':
		case ProfileOpt: {
			char *endptr;
			int profile = strtol (optarg, &endptr, 10);
			if (*endptr != '\0' || profile < 1 || profile > 3) {
				fprintf (stderr, "Invalid profile value: %s.\n", optarg);
				return EXIT_FAILURE;
			}
			target = profile;
			break;
		}

		case 'h':
		case HelpOpt:
			fprintf (stderr, usage, argv[0]);
			return EXIT_SUCCESS;

		default:
			return EXIT_FAILURE;
		}
	}

	if (argc-optind != 3) {
		fprintf (stderr, "Invalid argument count.\n");
		fprintf (stderr, usage, argv[0]);
		return EXIT_FAILURE;
	}
	
	for (int i = 0; i < 3; ++i) {
		char *endptr;
		color[i] = strtol (argv[optind+i], &endptr, 0);
		if (*endptr != '\0' || color[i] < 0 || color[i] > 255) {
			fprintf (stderr, "Invalid color value: %s\n", argv[optind+i]);
			return EXIT_FAILURE;
		}
	}

	/*
	 * Init libusb context and device
	 */
	int ret;
	bool error = false;
	libusb_context *context;
	if (0 != (ret = libusb_init (&context))) {
		fprintf (stderr, "Failed to initialize libusb: %s\n", libusb_error_name (ret));
		return EXIT_FAILURE;
	}
	libusb_set_debug (context, LIBUSB_LOG_LEVEL_WARNING);

	libusb_device_handle *device;
	if (!(device = libusb_open_device_with_vid_pid (context, CORSAIR_ID, CORSAIR_K40_ID))) {
		fprintf (stderr, "Device not found\n");
		return EXIT_FAILURE;
	}

	ret = libusb_control_transfer (device,
				       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
				       REQUEST_SET_COLOR, color[1] << 8 | color[2], target << 8 | color[3],
				       nullptr, 0, 0);
	if (ret < 0) {
		fprintf (stderr, "Failed to get hardware mode: %s (%s)\n",
				 libusb_error_name (ret), strerror (errno));
		error = true;
	}

	/*
	 * Clean up libusb
	 */
	libusb_close (device);
	libusb_exit (context);

	return (error ? EXIT_FAILURE : EXIT_SUCCESS);
}
