/*
 * ws2812x.c: Simple code for bit-banging LED control protocol
 *
 * No rights reserved, released to the public domain.
 * Written by Calvin Owens <jcalvinowens@gmail.com>
 */

#define NS2C(v) ((((long)F_CPU * (v)) + 1000000000L - 1L) / 1000000000L)

static void ws2812x_repeat_d2(uint8_t *data, int len, int repeats)
{
	int c;

	cli();
	for (c = 0; c < repeats * len; c++) {
		const uint8_t val = data[c % len];
		int i;

		for (i = 7; i >= 0; i--) {
			if ((val >> i) & 1) {
				VPORTA.OUT |= 1U << 0;
				__builtin_avr_delay_cycles(NS2C(500));
				VPORTA.OUT &= ~(1U << 0);
				__builtin_avr_delay_cycles(NS2C(125));
			} else {
				VPORTA.OUT |= 1U << 0;
				__builtin_avr_delay_cycles(NS2C(125));
				VPORTA.OUT &= ~(1U << 0);
				__builtin_avr_delay_cycles(NS2C(188));
			}
		}
	}
	sei();

	delayMicroseconds(50);
}

struct ws2812x_msg {
	uint8_t G;
	uint8_t R;
	uint8_t B;
};

void setup(void)
{
	pinMode(2, OUTPUT);
	digitalWrite(2, LOW);
}

void loop(void)
{
	struct ws2812x_msg off = {0};
	struct ws2812x_msg on = {
		.G = 255,
		.R = 255,
		.B = 255,
	};

	ws2812x_repeat_d2((uint8_t *)&on, 3, 144);
	delay(5000);

	ws2812x_repeat_d2((uint8_t *)&off, 3, 144);
	delay(5000);
}
