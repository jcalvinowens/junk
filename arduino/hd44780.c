/*
 * hd44780.c: Display serial input on HD44780 dot matrix screens.
 *
 * No rights reserved, released to the public domain.
 * Written by Calvin Owens <jcalvinowens@gmail.com>
 */

#include <Wire.h>

enum {
	PCF_RS = 0x01,
	PCF_EN = 0x04,
	PCF_BL = 0x08,
};

static void hd44780_i2c_send(uint8_t val, uint8_t flags)
{
	Wire.beginTransmission(0x27);
	Wire.write((val << 4) | flags);
	Wire.endTransmission();
}

static void hd44780_i2c_send_nibble(uint8_t val, uint8_t flags)
{
	hd44780_i2c_send(val, PCF_EN | flags);
	delayMicroseconds(1);
	hd44780_i2c_send(val, flags);
	delayMicroseconds(37);
}

static void hd44780_i2c_send_cmd(uint8_t val, uint8_t flags)
{
	hd44780_i2c_send_nibble(val >> 4, flags);
	hd44780_i2c_send_nibble(val & 0x0F, flags);
}

static void hd44780_print_char(char c)
{
	hd44780_i2c_send_cmd(c, PCF_RS | PCF_BL);
}

static void hd44780_print_row(int row, const char *str)
{
	const int row_offsets[4] = {0, 64, 20, 84};
	char c;

	hd44780_i2c_send_cmd(0x80 | row_offsets[row], PCF_BL);
	delayMicroseconds(37);

	while ((c = *str++)) {
		if (c < ' ' || c > '~')
			c = ' ';

		hd44780_print_char(c);
	}
}

static void hd44780_clear(void)
{
	hd44780_i2c_send_cmd(0x01, PCF_BL);
	delayMicroseconds(1600);
}

static void hd44780_flash(int c)
{
	while (c--) {
		hd44780_i2c_send_cmd(0x00, 0);
		delay(100);
		hd44780_i2c_send_cmd(0x00, PCF_BL);
		delay(100);
	}
}

static void hd44780_init(void)
{
	hd44780_i2c_send(0x00, PCF_BL);
	delayMicroseconds(50000);
	hd44780_i2c_send_nibble(0x03, PCF_BL);
	delayMicroseconds(4500);
	hd44780_i2c_send_nibble(0x03, PCF_BL);
	delayMicroseconds(200);
	hd44780_i2c_send_nibble(0x03, PCF_BL);
	delayMicroseconds(200);
	hd44780_i2c_send_nibble(0x02, PCF_BL);

	hd44780_i2c_send_cmd(0x2C, PCF_BL);
	hd44780_i2c_send_cmd(0x0C, PCF_BL);
	hd44780_i2c_send_cmd(0x01, PCF_BL);
	delayMicroseconds(1600);

	hd44780_i2c_send_cmd(0x06, PCF_BL);
	delayMicroseconds(1600);
}

void setup() {
	Serial.begin(9600, SERIAL_8N1);
	Serial.setTimeout(100);

	Wire.begin();
	Wire.setClock(100000);
	hd44780_init();
	hd44780_clear();
	hd44780_print_row(0, "	Monitoring UART...");
	hd44780_print_row(1, " Flashed " __DATE__);
}

static unsigned long i;
static char lines[4][21] = {
	"										",
	"										",
	"										",
	"										",
};

void loop() {
	int ret = Serial.readBytes(&lines[i % 4][0], 20);
	if (!ret)
		return;

	if (ret == 1 && (lines[i % 4][0] < ' ' || lines[i % 4][0] > '~'))
		return;

	hd44780_print_row(0, &lines[(i + 1) % 4][0]);
	hd44780_print_row(1, &lines[(i + 2) % 4][0]);
	hd44780_print_row(2, &lines[(i + 3) % 4][0]);
	hd44780_print_row(3, &lines[i % 4][0]);

	memset(&lines[++i % 4][0], ' ', 20);
}
