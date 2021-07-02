void setup(void)
{
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, LOW);
}

void loop(void)
{
	static uint8_t ctr;

	digitalWrite(LED_BUILTIN, ctr++ & 1 ? HIGH : LOW);
	delay(1000);
}
