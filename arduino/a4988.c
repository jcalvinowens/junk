/*
 * a4988.c: Simple serial interface to drive a stepper motor.
 *
 * No rights reserved, released to the public domain.
 * Written by Calvin Owens <jcalvinowens@gmail.com>
 */

static const int gpio_enable = 2;
static const int gpio_reset = 6;
static const int gpio_sleep = 7;
static const int gpio_step = 8;
static const int gpio_dir = 9;
static const int gpio_ms1 = 3;
static const int gpio_ms2 = 4;
static const int gpio_ms3 = 5;

/*
 * https://www.digikey.com/htmldatasheets/production/693406/0/0/1/a4988.html
 */

enum {
	ENA_ENABLE = LOW,
	ENA_DISABLE = HIGH,
};

enum {
	RESET_NORMAL = HIGH,
	RESET_TRIGGER = LOW,
};

enum {
	SLEEP_RUN = HIGH,
	SLEEP_ZZZ = LOW,
};

enum {
	MS_FULL = 0,
	MS_2ND	= 1,
	MS_4TH	= 2,
	MS_8TH	= 3,
	MS_16TH = 7,
};

enum {
	DIR_CW = HIGH,
	DIR_CCW = LOW,
};

/*
 * Global state
 */

static int current_enable = ENA_DISABLE;
static int current_sleep = SLEEP_ZZZ;
static int current_ms = MS_FULL;
static int current_dir = DIR_CW;
static int current_delay_ms = 1;

/*
 * A4988 control
 */

static void a4988_reset(void)
{
	digitalWrite(gpio_reset, RESET_TRIGGER);
	delay(current_delay_ms);

	digitalWrite(gpio_reset, RESET_NORMAL);
}

static void a4988_enable(void)
{
	if (current_enable == ENA_ENABLE)
		return;

	current_enable = ENA_ENABLE;
	digitalWrite(gpio_enable, ENA_ENABLE);
	delay(current_delay_ms);
}

static void a4988_disable(void)
{
	current_enable = ENA_DISABLE;
	digitalWrite(gpio_enable, ENA_DISABLE);
}

static void a4988_sleep(void)
{
	current_sleep = SLEEP_ZZZ;
	digitalWrite(gpio_sleep, SLEEP_ZZZ);
}

static void a4988_wake(void)
{
	if (current_sleep == SLEEP_RUN)
		return;

	current_sleep = SLEEP_RUN;
	digitalWrite(gpio_sleep, SLEEP_RUN);
	delay(current_delay_ms);
}

static void a4988_direction(int dir)
{
	if (dir == current_dir)
		return;

	current_dir = dir;
	digitalWrite(gpio_dir, dir);
	delay(current_delay_ms);
}

static void __a4988_resolution(int res)
{
	digitalWrite(gpio_ms1, res & 1 ? HIGH : LOW);
	digitalWrite(gpio_ms2, res & 2 ? HIGH : LOW);
	digitalWrite(gpio_ms3, res & 4 ? HIGH : LOW);
}

static void a4988_resolution(int ms)
{
	if (ms == current_ms)
		return;

	current_ms = ms;
	__a4988_resolution(ms);
	delay(current_delay_ms);
}

static void a4988_step(int dir, long steps, int res, long us)
{
	long i;

	steps = steps > 0 ? steps : -steps;
	a4988_resolution(res);
	a4988_direction(dir);

	for (i = 0; i < steps; i++) {
		digitalWrite(gpio_step, HIGH);
		delayMicroseconds(us);
		digitalWrite(gpio_step, LOW);
		delayMicroseconds(us);
	}
}

/*
 * Serial protocol
 */

struct serial_command {
	uint8_t divs;
	uint8_t disable;
	uint8_t sleep;
	uint8_t reset;

	int16_t magic;
	int16_t delay_ms;
	int32_t period_us;
	int32_t steps;
};

static void run_serial_command(void)
{
	struct serial_command cmd = {0};
	size_t ret;

	ret = Serial.readBytes((char *)&cmd, sizeof(cmd));
	if (ret == 0) {
		Serial.print("Timeout, no command!\n");
		return;
	}

	if (cmd.magic != 69) {
		Serial.print("Bad magic: ");
		Serial.print(cmd.magic);
		Serial.print("! Ignoring...\n");
		return;
	}

	Serial.print("CMD: divs=");
	Serial.print(cmd.divs);
	Serial.print(", DSR=");
	Serial.print(cmd.disable);
	Serial.print(cmd.sleep);
	Serial.print(cmd.reset);
	Serial.print(", delay_ms=");
	Serial.print(cmd.delay_ms);
	Serial.print(", period_us=");
	Serial.print(cmd.period_us);
	Serial.print(", steps=");
	Serial.print(cmd.steps);
	Serial.print("\n");

	if (cmd.delay_ms > 0)
		current_delay_ms = cmd.delay_ms;

	if (cmd.reset)
		a4988_reset();

	if (cmd.steps != 0) {
		a4988_wake();
		a4988_enable();
		a4988_step(cmd.steps < 0 ? DIR_CCW : DIR_CW,
			   cmd.steps, cmd.divs, cmd.period_us);
	}

	if (cmd.disable)
		a4988_disable();

	if (cmd.sleep)
		a4988_sleep();
}

void setup()
{
	pinMode(gpio_enable, OUTPUT);
	digitalWrite(gpio_enable, current_enable);

	pinMode(gpio_sleep, OUTPUT);
	digitalWrite(gpio_sleep, current_sleep);

	pinMode(gpio_reset, OUTPUT);
	digitalWrite(gpio_reset, RESET_NORMAL);

	pinMode(gpio_step, OUTPUT);
	digitalWrite(gpio_step, LOW);

	pinMode(gpio_dir, OUTPUT);
	digitalWrite(gpio_dir, current_dir);

	pinMode(gpio_ms1, OUTPUT);
	pinMode(gpio_ms2, OUTPUT);
	pinMode(gpio_ms3, OUTPUT);
	__a4988_resolution(current_ms);

	Serial.begin(9600, SERIAL_8N1);
	Serial.setTimeout(1000);
	Serial.print("I live!\n");

	delay(5000);
}

void loop()
{
	run_serial_command();
}
