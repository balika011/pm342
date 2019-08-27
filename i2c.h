#ifndef __I2C_H__
#define __I2C_H__

#include <stdint.h>
#include "mpsse.h"

typedef struct 
{
	mpsse_context *mpsse;
	struct
	{
		uint8_t val;
		uint8_t dir;
	} upper_bits;
} i2c_context;

int i2c_get_devices(int vid, int pid, const char ***descriptions, const char ***serials, unsigned int *num_devices);
i2c_context *i2c_new(int vid, int pid, const char *description, const char *serial, unsigned int index, ftdi_interface interface, uint8_t upper_bits_val, uint8_t upper_bits_dir, uint8_t upper_bits_mask);
void i2c_free(i2c_context *i2c);

int i2c_read(i2c_context *i2c, uint8_t addr, uint8_t reg, uint8_t *data);
int i2c_write(i2c_context *i2c, uint8_t addr, uint8_t reg, uint8_t data);
uint8_t i2c_get_upper_bits_val(i2c_context *i2c);
uint8_t i2c_get_upper_bits_dir(i2c_context *i2c);
void i2c_set_upper_bits(i2c_context *i2c, uint8_t dir, uint8_t val);

#endif