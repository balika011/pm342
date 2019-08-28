#include <stdlib.h>

#include "i2c.h"

int i2c_get_devices(int vid, int pid, const char ***descriptions, const char ***serials, unsigned int *num_devices)
{
	return mpsse_get_devices(vid, pid, descriptions, serials, num_devices);
}

i2c_context *i2c_new(int vid, int pid, const char *description, const char *serial, unsigned int index, ftdi_interface interface, uint8_t upper_bits_val, uint8_t upper_bits_dir, uint8_t upper_bits_mask)
{
	i2c_context *i2c = (i2c_context *) malloc(sizeof(i2c_context));
	if (i2c)
	{
		i2c->mpsse = mpsse_new(vid, pid, description, serial, index, interface);
		if (i2c->mpsse)
		{
			uint8_t buf[3];
			buf[0] = GET_BITS_LOW;
			buf[1] = 0x87;
			mpsse_queue(i2c->mpsse, buf, 2);
			mpsse_flush(i2c->mpsse);

			int read = 0;
			for (int i = 0; i <= 4; ++i)
			{
				uint8_t val;
				int count = mpsse_raw_read(i2c->mpsse, &val, 1);
				if (count < 0)
					break;
				if (count > 0)
				{
					read = 1;
					val >>= 4;
					i2c->upper_bits.val = (upper_bits_val | upper_bits_mask & val) << 4;
					i2c->upper_bits.dir = 0xF0;
					break;
				}
			}

			if (read)
			{
				uint8_t buf_0[6];
				buf[0] = DIS_DIV_5;
				buf[1] = DIS_ADAPTIVE;
				buf[2] = DIS_3_PHASE;
				mpsse_queue(i2c->mpsse, buf, 3);
				mpsse_flush(i2c->mpsse);

				buf_0[0] = SET_BITS_LOW;
				buf_0[1] = i2c->upper_bits.val | 3;
				buf_0[2] = i2c->upper_bits.dir | 3;

				buf_0[3] = TCK_DIVISOR;
				buf_0[4] = 0x95; // ~200 KHz
				buf_0[5] = 0;
				mpsse_queue(i2c->mpsse, buf_0, 6);
				mpsse_flush(i2c->mpsse);

				buf[0] = LOOPBACK_END;
				mpsse_queue(i2c->mpsse, buf, 1);
				mpsse_flush(i2c->mpsse);
				return i2c;
			}
		}
	}

	if (i2c)
	{
		free(i2c);
		i2c = 0;
	}
	return 0;
}

void i2c_free(i2c_context *i2c)
{
	mpsse_free(i2c->mpsse);
	free(i2c);
}

void start_bit(i2c_context *i2c)
{
	uint8_t buf[3];
	for (int i = 0; i <= 3; ++i)
	{
		buf[0] = SET_BITS_LOW;
		buf[1] = i2c->upper_bits.val | 3;
		buf[2] = i2c->upper_bits.dir | 3;
		mpsse_queue(i2c->mpsse, buf, 3);
	}

	for (int i = 0; i <= 3; ++i)
	{
		buf[0] = SET_BITS_LOW;
		buf[1] = i2c->upper_bits.val | 1;
		buf[2] = i2c->upper_bits.dir | 3;
		mpsse_queue(i2c->mpsse, buf, 3);
	}

	buf[0] = SET_BITS_LOW;
	buf[1] = i2c->upper_bits.val;
	buf[2] = i2c->upper_bits.dir | 3;
	mpsse_queue(i2c->mpsse, buf, 3);
}

void stop_bit(i2c_context *i2c)
{
	uint8_t buf[3];
	for (int i = 0; i <= 3; ++i)
	{
		buf[0] = SET_BITS_LOW;
		buf[1] = i2c->upper_bits.val | 1;
		buf[2] = i2c->upper_bits.dir | 3;
		mpsse_queue(i2c->mpsse, buf, 3);
	}

	for (int i = 0; i <= 3; ++i)
	{
		buf[0] = SET_BITS_LOW;
		buf[1] = i2c->upper_bits.val | 3;
		buf[2] = i2c->upper_bits.dir | 3;
		mpsse_queue(i2c->mpsse, buf, 3);
	}

	buf[0] = SET_BITS_LOW;
	buf[1] = i2c->upper_bits.val;
	buf[2] = i2c->upper_bits.dir | 0x10;
	mpsse_queue(i2c->mpsse, buf, 3);
}

int send_byte_check_ack(i2c_context *i2c, uint8_t data)
{
	uint8_t buf[6];
	buf[0] = 0x11; // MSB_FALLING_EDGE_CLOCK_BYTE_OUT
	buf[1] = 0;
	buf[2] = 0;
	buf[3] = data; // Data length of 0x0000 means 1 byte data to clock out
	mpsse_queue(i2c->mpsse, buf, 4);

	buf[0] = SET_BITS_LOW;
	buf[1] = i2c->upper_bits.val | 0x00; // Set SCL low
	buf[2] = i2c->upper_bits.dir | 0x11;

	buf[3] = 0x22; // MSB_RISING_EDGE_CLOCK_BIT_IN
	buf[4] = 0;    // Length of 0x0 means to scan in 1 bit
	buf[5] = 0x87; // Send answer back immediate command
	mpsse_queue(i2c->mpsse, buf, 6);
	mpsse_flush(i2c->mpsse);

	int status = 1;
	for (int i = 0; i <= 4; ++i)
	{
		uint8_t ack;
		int count = mpsse_raw_read(i2c->mpsse, &ack, 1);
		if (count < 0)
			break;

		if (count > 0 && !(ack & 1))
		{
			status = 0;
			break;
		}
	}

	buf[0] = SET_BITS_LOW;
	buf[1] = i2c->upper_bits.val | 2; // Set SDA high, SCL low
	buf[2] = i2c->upper_bits.dir | 3; // Set SK,DO,GPIOL0 pins as output
	mpsse_queue(i2c->mpsse, buf, 3);
	return status;
}

int read_byte(i2c_context *i2c, uint8_t *data)
{
	uint8_t buf[9];
	buf[0] = SET_BITS_LOW;
	buf[1] = i2c->upper_bits.val; // Set SCL low
	buf[2] = i2c->upper_bits.dir | 0x11; // Set SK, GPIOL0 pins as output

	buf[3] = 0x24; // MSB_FALLING_EDGE_CLOCK_BYTE_IN 
	buf[4] = 0;
	buf[5] = 0; // Data length of 0x0000 means 1 byte data to clock in

	buf[6] = 0x22; // MSB_RISING_EDGE_CLOCK_BIT_IN 
	buf[7] = 0;    // Length of 0x0 means to scan in 1 bit
	buf[8] = 0x87; // Send answer back immediate command
	mpsse_queue(i2c->mpsse, buf, 9);
	mpsse_flush(i2c->mpsse);

	for (int i = 0; i <= 4; ++i )
	{
		uint8_t buf_0[2];
		int count = mpsse_raw_read(i2c->mpsse, buf_0, 2);
		if (count == 2)
		{
			*data = buf_0[0];
			return 0;
		}
	}

	return 1;
}

int i2c_read(i2c_context *i2c, uint8_t addr, uint8_t reg, uint8_t *data)
{
	start_bit(i2c);
	int status = send_byte_check_ack(i2c, addr << 1);
	if (!status)
	{
		status = send_byte_check_ack(i2c, reg);
		if (!status)
		{
			stop_bit(i2c);
			start_bit(i2c);
			status = send_byte_check_ack(i2c, (addr << 1) | 1);
			if (!status)
				status = read_byte(i2c, data);
		}
	}
	stop_bit(i2c);
	mpsse_flush(i2c->mpsse);
	return status;
}

int i2c_write(i2c_context *i2c, uint8_t addr, uint8_t reg, uint8_t data)
{
	start_bit(i2c);
	int status = send_byte_check_ack(i2c, addr << 1);
	if (!status)
	{
		status = send_byte_check_ack(i2c, reg);
		if (!status)
			status = send_byte_check_ack(i2c, data);
	}
	stop_bit(i2c);
	mpsse_flush(i2c->mpsse);

	return status;
}

uint8_t i2c_get_upper_bits_val(i2c_context *i2c)
{
	return i2c->upper_bits.val >> 4;
}

uint8_t i2c_get_upper_bits_dir(i2c_context *i2c)
{
	return i2c->upper_bits.dir >> 4;
}

void i2c_set_upper_bits(i2c_context *i2c, uint8_t dir, uint8_t val)
{
	uint8_t buf[3];

	i2c->upper_bits.val = val << 4;
	i2c->upper_bits.dir = dir << 4;
	buf[0] = SET_BITS_LOW;
	buf[1] = i2c->upper_bits.val | 3;
	buf[2] = i2c->upper_bits.dir | 3;
	mpsse_queue(i2c->mpsse, buf, 3);
	mpsse_flush(i2c->mpsse);
}
