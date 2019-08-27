#include <unistd.h>
#include "pm342.h"

int pm342_get_devices(const char ***descriptions, const char ***serials, unsigned int *num_devices)
{
	return i2c_get_devices(0x403, 0x6011, descriptions, serials, num_devices);
}

pm342_context *pm342_new_desc(int vid, int pid, const char *description, const char *serial, unsigned int index)
{
	pm342_context *pm342 = (pm342_context *) malloc(sizeof(pm342_context));
	if (pm342)
	{
		pm342->i2c = i2c_new(vid, pid, description, serial, index, INTERFACE_A, 0, 0, 0xF);
		if (pm342->i2c)
			return pm342;
	}
	if (pm342)
	{
		if (pm342->i2c)
		{
			i2c_free(pm342->i2c);
			pm342->i2c = 0;
		}
		free(pm342);
	}
	return 0;
}

pm342_context *pm342_new(char *product, char *serial, unsigned int index)
{
	return pm342_new_desc(0x403, 0x6011, product, serial, index);
}

void pm342_free(pm342_context *pm342)
{
	i2c_free(pm342->i2c);
	free(pm342);
}

int pm342_toggle_keys(pm342_context *pm342, PM342PhidgetKeys keys, int state, int timeout)
{
	int status = pm342_set_keys(pm342, keys, state);
	if (!status && timeout > 0)
	{
		usleep(timeout);
		status = pm342_set_keys(pm342, keys, ~state);
	}
	return status;
}

int pm342_set_keys(pm342_context *pm342, PM342PhidgetKeys keys, int state)
{
	uint8_t configPort1;
	uint8_t outputPort1;

	if (keys & PM342PK_RESET)
	{
		if (i2c_read(pm342->i2c, 0x74, 6, &configPort1))
			return 1;

		if (i2c_read(pm342->i2c, 0x74, 2, &outputPort1))
			return 1;

		if (state & PM342PK_RESET)
		{
			configPort1 &= ~8;
			outputPort1 &= ~8;
		}
		else
		{
			configPort1 |= 8;
			outputPort1 |= 8;
		}

		if (i2c_write(pm342->i2c, 0x74, 6, configPort1))
			return 1;
		if (i2c_write(pm342->i2c, 0x74, 2, outputPort1))
			return 1;
	}

	if (keys & (PM342PK_ONKEY | PM342PK_FORCE_RECOVERY))
	{
		if (i2c_read(pm342->i2c, 0x74, 7, &configPort1))
			return 1;
		if (i2c_read(pm342->i2c, 0x74, 3, &outputPort1))
			return 1;

		if (keys & PM342PK_ONKEY)
		{
			if (state & PM342PK_ONKEY)
			{
				configPort1 &= ~0x10;
				outputPort1 &= ~0x10;
			}
			else
			{
				configPort1 |= 0x10;
				outputPort1 |= 0x10;
			}
		}

		if (keys & PM342PK_FORCE_RECOVERY)
		{
			if (state & PM342PK_FORCE_RECOVERY)
			{
				configPort1 &= ~8;
				outputPort1 &= ~8;
			}
			else
			{
				configPort1 |= 8;
				outputPort1 |= 8;
			}
		}

		if (i2c_write(pm342->i2c, 0x74, 7, configPort1))
			return 1;
		if (i2c_write(pm342->i2c, 0x74, 3, outputPort1))
			return 1;
	}

	if (keys & PM342PK_FORCE_OFF)
	{
		i2c_set_upper_bits(pm342->i2c,
			i2c_get_upper_bits_dir(pm342->i2c) | 4,
			state & PM342PK_FORCE_OFF ? i2c_get_upper_bits_val(pm342->i2c) & 0xB :
			i2c_get_upper_bits_val(pm342->i2c) | 4);
	}
	
	if (keys & PM342PK_USB)
	{
		i2c_set_upper_bits(pm342->i2c,
			i2c_get_upper_bits_dir(pm342->i2c) | 8,
				state & PM342PK_USB ? i2c_get_upper_bits_val(pm342->i2c) | 8 :
				i2c_get_upper_bits_val(pm342->i2c) & 7);
	}
	return 0;
}

int pm342_get_status(pm342_context *pm342, int *vdd_core_rail, int *vdd_cpu_rail)
{
	uint8_t configPort;
	if (i2c_read(pm342->i2c, 0x74, 7, &configPort))
		return 1;

	configPort |= 0xC0;
	if (i2c_write(pm342->i2c, 0x74, 7, configPort) )
		return 1;

	uint8_t inputPort;
	if (i2c_read(pm342->i2c, 0x74, 1, &inputPort))
		return 1;

	*vdd_core_rail = (inputPort & 0x40) == 0;
	*vdd_cpu_rail = ~inputPort >> 7;

	return 0;
}
