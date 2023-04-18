/*
 * ws2811d.c: Simple daemon for writing UDP RGB buffers to WS2811 LED strings
 *	      on Raspberry Pis using https://github.com/jgarff/rpi_ws281x.git
 *
 * gcc -I./rpi_ws281x/ ws2811d.c ./rpi_ws281x/build/libws2811.a -lm
 *
 * Copyright (C) 2022 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ws2811.h>

struct args {
	int wait_time_us;
	int freq;
	int led_count;
	int gpio;
	int dma;
};

static void run_leds(const struct args *a)
{
	const struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_addr = htonl((INADDR_ANY)),
		.sin_port = htons(1337),
	};
	ws2811_t config = {
		.freq = a->freq,
		.render_wait_time = a->wait_time_us,
		.dmanum = a->dma,
		.channel = {
			{
				.gpionum = a->gpio,
				.count = a->led_count,
				.invert = 0,
				.brightness = 255,
				.strip_type = WS2811_STRIP_GRB,
			},
		},

	};
	int sockfd;

	if (ws2811_init(&config) != WS2811_SUCCESS) {
		puts("WS2811 failure!");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		printf("Can't socket: %m");
		exit(1);
	}

	if (bind(sockfd, (const struct sockaddr *)&sa, sizeof(sa)) == -1) {
		printf("Can't bind: %m\n");
		exit(1);
	}

	while (1) {
		uint8_t buf[a->led_count * 3];
		int i;

		if (recv(sockfd, buf, sizeof(buf), 0) != (signed)sizeof(buf))
			continue;

		for (i = 0; i < a->led_count; i++)
			config.channel[0].leds[i] =
				((uint32_t)buf[i * 3 + 0] << 16) |	// Red
				((uint32_t)buf[i * 3 + 1] << 8) |	// Green
				((uint32_t)buf[i * 3 + 2] << 0);	// Blue

		ws2811_render(&config);
	}

}

static const char *usage_fmt = "Usage: ./%s --led-count N [--gpio G]"
"[--frequency F][--wait-time-us W][--dma D]\n\n";

static const char *optstr = "hc:g:f:w:d:";
static const struct option optlong[] = {
	{
		.name = "led-count",
		.has_arg = required_argument,
		.val = 'c',
	},
	{
		.name = "gpio",
		.has_arg = required_argument,
		.val = 'g',
	},
	{
		.name = "frequency",
		.has_arg = required_argument,
		.val = 'f',
	},
	{
		.name = "wait-time-us",
		.has_arg = required_argument,
		.val = 'w',
	},
	{
		.name = "dma",
		.has_arg = required_argument,
		.val = 'd',
	},
	{
		.name = "help",
		.has_arg = no_argument,
		.val = 'h',
	},
	{
		.name = NULL,
	},
};

int main(int argc, char **argv)
{
	struct args args = {
		.wait_time_us = 1000,
		.freq = 800000,
		.led_count = 0,
		.gpio = 18,
		.dma = 10,
	};

	while (1) {
		int i = getopt_long(argc, argv, optstr, optlong, NULL);

		switch (i) {
		case 'c':
			args.led_count = atoi(optarg);
			break;
		case 'g':
			args.gpio = atoi(optarg);
			break;
		case 'f':
			args.freq = atoi(optarg);
			break;
		case 'w':
			args.wait_time_us = atoi(optarg);
			break;
		case 'd':
			args.dma = atoi(optarg);
			break;
		case -1:
			goto done;
		case 'h':
			printf(usage_fmt, argv[0]);
		default:
			exit(1);
		}
	}
done:

	if (args.led_count == 0) {
		puts("You must pass --led-count!\n");
		return EXIT_FAILURE;
	}

	run_leds(&args);
	return 0;
}
