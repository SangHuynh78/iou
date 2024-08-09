/*
 * LSM6DSOX.c
 *
 * Created: 6/14/2024 1:19:01 AM
 *  Author: HTSANG
 */ 

#include "LSM6DSOX.h"
#include "i2c.h"
#include "math.h"
#include "twi.h"
#include "uart.h"
#include "scheduler.h"
#include "IOU_board.h"
#include "BMP390.h"

TWI_ERROR TWI_ERROR_GYRO = TWI_SUCCESS;

void lsm6dsox_write(uint8_t _reg, uint8_t _value) {
	twi_start();
	twi_write(LSM6DSOX_ADDRESS << 1);
	twi_write(_reg);
	twi_write(_value);
	twi_stop();
}

void lsm6dsox_read(uint8_t reg, uint8_t *buffer, uint8_t length) {
	twi_start();
	twi_write(LSM6DSOX_ADDRESS << 1);
	twi_write(reg);
	twi_start();
	twi_write((LSM6DSOX_ADDRESS << 1) | 1);
	for (uint8_t i = 0; i < length - 1; i++) {
		buffer[i] = twi_read_ack();
		if (buffer[i] >> 8) TWI_ERROR_GYRO = TWI_ERROR_NO_ACK;
	}
	buffer[length - 1] = twi_read_nack();
	if (buffer[length - 1] >> 8) TWI_ERROR_GYRO = TWI_ERROR_NO_ACK;
	twi_stop();
}

bool lsm6dsox_init(void) {
	// Accelerometer: 104 Hz, 8g ~ 78.48 m/s^2
	TWI_ERROR_GYRO=TWI_SUCCESS;
	lsm6dsox_write(LSM6DSOX_CTRL1_XL, 0x58);
	// Gyroscope: 104 Hz, 500 dps	
	lsm6dsox_write(LSM6DSOX_CTRL2_G, 0x54);
	// Read to control status
	twi_start();
	twi_write(LSM6DSOX_ADDRESS << 1);
	twi_write(LSM6DSOX_OUTX_L_G);
	twi_start();
	twi_write((LSM6DSOX_ADDRESS << 1) | 1);
	if (twi_read_nack() >> 8)
	{
		TWI_ERROR_GYRO = TWI_ERROR_NO_ACK;
		twi_stop();
		return false;
	}
 	return true;
}

void read_accel_gyro(Accel_Gyro_DataTypedef* _accel, Accel_Gyro_DataTypedef* _gyro)
{
	uint8_t buffer[12];
	lsm6dsox_read(LSM6DSOX_OUTX_L_G, buffer, 12);
	if (TWI_ERROR_GYRO == TWI_SUCCESS)
	{
		_gyro->x = (int16_t)((buffer[1] << 8 | buffer[0]) * GYRO_SENSITIVITY_500DPS / 1000.0f);
		_gyro->y = (int16_t)((buffer[3] << 8 | buffer[2]) * GYRO_SENSITIVITY_500DPS / 1000.0f);
		_gyro->z = (int16_t)((buffer[5] << 8 | buffer[4]) * GYRO_SENSITIVITY_500DPS / 1000.0f);
		_accel->x = (int16_t)((buffer[7] << 8 | buffer[6])   * LSM6DSOX_ACCL_FS_8G);
		_accel->y = (int16_t)((buffer[9] << 8 | buffer[8])   * LSM6DSOX_ACCL_FS_8G);
		_accel->z = (int16_t)((buffer[11] << 8 | buffer[10]) * LSM6DSOX_ACCL_FS_8G);
	}
	else
	{
		_gyro->x = 0x7FFF;
		_gyro->y = 0x7FFF;
		_gyro->z = 0x7FFF;
		_accel->x = 0x7FFF;
		_accel->y = 0x7FFF;
		_accel->z = 0x7FFF;
	}
}
