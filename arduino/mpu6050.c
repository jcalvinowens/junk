/*
 * mpu6050.c: Useless example code for collecting accel/gyro data.
 *
 * No rights reserved, released to the public domain.
 * Written by Calvin Owens <jcalvinowens@gmail.com>
 */

#include <Wire.h>

enum {
	SMPLRT_DIV = 0x19,
	CONFIG = 0x1A,
	GYRO_CONFIG = 0x1B,
	ACCEL_CONFIG = 0x1C,
	PWR_MGMT_1 = 0x6B,

	I2C_ADDR_0 = 0x68,
	I2C_ADDR_1 = 0x69,
};

static void mpu6050_write_register(uint8_t reg, uint8_t val)
{
	Wire.beginTransmission(I2C_ADDR_0);
	Wire.write(reg);
	Wire.write(val);
	Wire.endTransmission();
}

static void mpu6050_read_registers(uint8_t reg, int nr, uint8_t *out)
{
	int i;

	Wire.beginTransmission(I2C_ADDR_0);
	Wire.write(reg);
	Wire.endTransmission();

	Wire.requestFrom(I2C_ADDR_0, nr, true);
	for (i = 0; i < nr; i++)
		*out++ = Wire.read();
}

struct mpu6050 {
	int16_t accl_x;
	int16_t accl_y;
	int16_t accl_z;
	int16_t temp;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
};

static void mpu6050_get_data(struct mpu6050 *out)
{
	uint8_t v[14];

	mpu6050_read_registers(0x3B, 14, v);
	out->accl_x = v[0] << 8 | v[1];
	out->accl_y = v[2] << 8 | v[3];
	out->accl_z = v[4] << 8 | v[5];
	out->temp = v[6] << 8 | v[7];
	out->gyro_x = v[8] << 8 | v[9];
	out->gyro_y = v[10] << 8 | v[11];
	out->gyro_z = v[12] << 8 | v[13];

	out->temp = out->temp / 340 + 37;
}

static void mpu6050_init(void)
{
	mpu6050_write_register(PWR_MGMT_1, 0);
	mpu6050_write_register(CONFIG, 6);
	mpu6050_write_register(GYRO_CONFIG, 0x10); // 0x00 - 0x18
	mpu6050_write_register(ACCEL_CONFIG, 0x18); // 0x00 - 0x18
	mpu6050_write_register(SMPLRT_DIV, 49);
}

void setup(void)
{
	Wire.begin();
	Wire.setClock(400000);
	mpu6050_init();
}

void loop(void)
{
	struct mpu6050 cur;

	mpu6050_get_data(&cur);
	delay(50);
}
