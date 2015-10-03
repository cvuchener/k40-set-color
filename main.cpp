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

#define DELAY	200000

#define REQUEST_STATUS	4
#define REQUEST_SET_ANIM	49
#define REQUEST_SET_COLOR_MODE	50
#define REQUEST_SET_COLOR	51
#define REQUEST_SET_COLOR_CONTROL	56

#define COLOR_CONTROL_SW	0x0600
#define COLOR_CONTROL_HW	0x0a00

void print_status (libusb_device_handle *device);

static const char *usage =
	"Usage: %s [options] red green blue\n"
	"Options are:\n"
	"	-c, --control=control	Set control mode: sw (software) or hw (hardware).\n"
	"	-p, --profile=num	Set color for profile num only.\n"
	"	-m, --mode=mode		Set color mode: true (True Color) or max (Max brightness).\n"
	"	-a, --anim=anim		Set animation: off, pulse, cycle.\n"
	"	-r, --read		Read the current color and exit.\n"
	"	-h, --help		Print this help message.\n"
	;

int main (int argc, char *argv[])
{
	if (argc == 1) {
		fprintf (stderr, usage, argv[0]);
		return EXIT_FAILURE;
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

	print_status (device);

	/*
	 * Parse arguments
	 */
	enum {
		ProfileOpt = 256,
		ModeOpt,
		ControlOpt,
		AnimOpt,
		ReadOpt,
		HelpOpt,
	};
	struct option longopts[] = {
		{ "profile", required_argument, nullptr, ProfileOpt },
		{ "control", required_argument, nullptr, ControlOpt },
		{ "anim", required_argument, nullptr, AnimOpt },
		{ "mode", required_argument, nullptr, ModeOpt },
		{ "read", no_argument, nullptr, ReadOpt },
		{ "help", no_argument, nullptr, HelpOpt },
		{ nullptr, 0, nullptr, 0 }
	};
	int target = 0;
	int color[3];

	int opt;
	while (-1 != (opt = getopt_long (argc, argv, "p:m:c:a:rh", longopts, nullptr))) {
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
		case 'c':
		case ControlOpt: {
			uint16_t control;
			if (strcmp (optarg, "sw") == 0)
				control = COLOR_CONTROL_SW;
			else if (strcmp (optarg, "hw") == 0)
				control = COLOR_CONTROL_HW;
			else {
				fprintf (stderr, "Invalid control mode: %s.\n", optarg);
				return EXIT_FAILURE;
			}
			ret = libusb_control_transfer (device,
						       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
						       REQUEST_SET_COLOR_CONTROL, control, 0,
						       nullptr, 0, 0);
			if (ret < 0) {
				fprintf (stderr, "Failed to set color control: %s (%s).\n",
					 libusb_error_name (ret), strerror (errno));
				return EXIT_FAILURE;
			}
			fprintf (stderr, "Set color control to 0x%04hX.\n", control);
			usleep (DELAY);
			print_status (device);
			break;
		}
		case 'a':
		case AnimOpt: {
			uint16_t anim;
			if (strcmp (optarg, "off") == 0)
				anim = 0;
			else if (strcmp (optarg, "pulse") == 0)
				anim = 1;
			else if (strcmp (optarg, "cycle") == 0)
				anim = 2;
			else {
				fprintf (stderr, "Invalid animation: %s.\n", optarg);
				return EXIT_FAILURE;
			}
			ret = libusb_control_transfer (device,
						       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
						       REQUEST_SET_ANIM, 0, anim,
						       nullptr, 0, 0);
			if (ret < 0) {
				fprintf (stderr, "Failed to set animation: %s (%s).\n",
					 libusb_error_name (ret), strerror (errno));
				return EXIT_FAILURE;
			}
			fprintf (stderr, "Set animation to 0x%04hX.\n", anim);
			usleep (DELAY);
			print_status (device);
			break;
		}
		case 'm':
		case ModeOpt: {
			uint16_t mode;
			if (strcmp (optarg, "true") == 0)
				mode = 0;
			else if (strcmp (optarg, "max") == 0)
				mode = 1;
			else {
				fprintf (stderr, "Invalid color mode: %s.\n", optarg);
				return EXIT_FAILURE;
			}
			ret = libusb_control_transfer (device,
						       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
						       REQUEST_SET_COLOR_MODE, mode, 0,
						       nullptr, 0, 0);
			if (ret < 0) {
				fprintf (stderr, "Failed to set color mode: %s (%s).\n",
					 libusb_error_name (ret), strerror (errno));
			}
			fprintf (stderr, "Set color mode to: 0x%04hX.\n", mode);
			usleep (DELAY);
			print_status (device);
			break;
		}
		case 'r':
		case ReadOpt: {
			uint8_t data[10];
			ret = libusb_control_transfer (device,
						       LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
						       REQUEST_STATUS, 0, 0,
						       data, sizeof (data), 0);
			if (ret < 0) {
				fprintf (stderr, "Failed to read status: %s (%s)\n",
					 libusb_error_name (ret), strerror (errno));
				error = true;
			}
			else
				printf ("%hhu %hhu %hhu\n", data[4], data[5], data[6]);
			goto libusb_cleanup;
		}

		case 'h':
		case HelpOpt:
			fprintf (stderr, usage, argv[0]);
			return EXIT_SUCCESS;

		default:
			return EXIT_FAILURE;
		}
	}

	if (argc-optind == 0)
		goto libusb_cleanup;

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
	 * Set LED color
	 */
	ret = libusb_control_transfer (device,
				       LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
				       REQUEST_SET_COLOR, color[1] << 8 | color[0], target << 8 | color[2],
				       nullptr, 0, 0);
	if (ret < 0) {
		fprintf (stderr, "Failed to set color: %s (%s)\n",
				 libusb_error_name (ret), strerror (errno));
		error = true;
	}
	if (target == 0)
		fprintf (stderr, "Set color %02hhX%02hhX%02hhX.\n", color[0], color[1], color[2]);
	else
		fprintf (stderr, "Set color %02hhX%02hhX%02hhX for profile %d.\n", color[0], color[1], color[2], target);

	/*
	 * Clean up libusb
	 */
libusb_cleanup:
	libusb_close (device);
	libusb_exit (context);

	return (error ? EXIT_FAILURE : EXIT_SUCCESS);
}

void print_status (libusb_device_handle *device)
{
	int ret;
	uint8_t data[10];

	ret = libusb_control_transfer (device,
				       LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
				       REQUEST_STATUS, 0, 0,
				       data, sizeof (data), 0);
	if (ret < 0) {
		fprintf (stderr, "Failed to read status: %s (%s)\n",
				 libusb_error_name (ret), strerror (errno));
	}
	else {
		fprintf (stderr, "Status:");
		for (unsigned int i = 0; i < sizeof (data); ++i)
			fprintf (stderr, " %02hhx", data[i]);
		fprintf (stderr, "\n");
	}
}
