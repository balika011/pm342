#include <unistd.h>
#include "pm342.h"

#define FT4232H_VID 0x0403
#define FT4232H_PID 0x6011

#define TCA9539_A0_L_A1_L_ADDR 0x74
#define TCA9539_A0_L_A1_H_ADDR 0x75
#define TCA9539_A0_H_A1_L_ADDR 0x76
#define TCA9539_A0_H_A1_H_ADDR 0x77

#define TCA9539_INPUT_PORT_0_REG 0x00
#define TCA9539_INPUT_PORT_1_REG 0x01
#define TCA9539_OUTPUT_PORT_0_REG 0x02
#define TCA9539_OUTPUT_PORT_1_REG 0x03
#define TCA9539_POLERITY_INVERSION_PORT_0_REG 0x04
#define TCA9539_POLERITY_INVERSION_PORT_1_REG 0x05
#define TCA9539_CONFIGURATION_PORT_0_REG 0x06
#define TCA9539_CONFIGURATION_PORT_1_REG 0x07

#define TCA9539_P00_FLAG (1 << 0)
#define TCA9539_P01_FLAG (1 << 1)
#define TCA9539_P02_FLAG (1 << 2)
#define TCA9539_P03_FLAG (1 << 3)
#define TCA9539_P04_FLAG (1 << 4)
#define TCA9539_P05_FLAG (1 << 5)
#define TCA9539_P06_FLAG (1 << 6)
#define TCA9539_P07_FLAG (1 << 7)

#define TCA9539_P10_FLAG (1 << 0)
#define TCA9539_P11_FLAG (1 << 1)
#define TCA9539_P12_FLAG (1 << 2)
#define TCA9539_P13_FLAG (1 << 3)
#define TCA9539_P14_FLAG (1 << 4)
#define TCA9539_P15_FLAG (1 << 5)
#define TCA9539_P16_FLAG (1 << 6)
#define TCA9539_P17_FLAG (1 << 7)

int pm342_get_devices(const char ***descriptions, const char ***serials, unsigned int *num_devices)
{
	return i2c_get_devices(FT4232H_VID, FT4232H_PID, descriptions, serials, num_devices);
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
	return pm342_new_desc(FT4232H_VID, FT4232H_PID, product, serial, index);
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
	uint8_t configPort;
	uint8_t outputPort;

	if (keys & PM342PK_RESET)
	{
		if (i2c_read(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_CONFIGURATION_PORT_0_REG, &configPort))
			return 1;

		if (i2c_read(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_OUTPUT_PORT_0_REG, &outputPort))
			return 1;

		if (state & PM342PK_RESET)
		{
			configPort &= ~TCA9539_P03_FLAG;
			outputPort &= ~TCA9539_P03_FLAG;
		}
		else
		{
			configPort |= TCA9539_P03_FLAG;
			outputPort |= TCA9539_P03_FLAG;
		}

		if (i2c_write(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_CONFIGURATION_PORT_0_REG, configPort))
			return 1;
		if (i2c_write(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_OUTPUT_PORT_0_REG, outputPort))
			return 1;
	}

	if (keys & (PM342PK_ONKEY | PM342PK_FORCE_RECOVERY))
	{
		if (i2c_read(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_CONFIGURATION_PORT_1_REG, &configPort))
			return 1;
		if (i2c_read(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_OUTPUT_PORT_1_REG, &outputPort))
			return 1;

		if (keys & PM342PK_ONKEY)
		{
			if (state & PM342PK_ONKEY)
			{
				configPort &= ~TCA9539_P14_FLAG;
				outputPort &= ~TCA9539_P14_FLAG;
			}
			else
			{
				configPort |= TCA9539_P14_FLAG;
				outputPort |= TCA9539_P14_FLAG;
			}
		}

		if (keys & PM342PK_FORCE_RECOVERY)
		{
			if (state & PM342PK_FORCE_RECOVERY)
			{
				configPort &= ~TCA9539_P13_FLAG;
				outputPort &= ~TCA9539_P13_FLAG;
			}
			else
			{
				configPort |= TCA9539_P13_FLAG;
				outputPort |= TCA9539_P13_FLAG;
			}
		}

		if (i2c_write(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_CONFIGURATION_PORT_1_REG, configPort))
			return 1;
		if (i2c_write(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_OUTPUT_PORT_1_REG, outputPort))
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
	if (i2c_read(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_CONFIGURATION_PORT_1_REG, &configPort))
		return 1;
	configPort |= TCA9539_P16_FLAG | TCA9539_P17_FLAG;
	if (i2c_write(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_CONFIGURATION_PORT_1_REG, configPort) )
		return 1;

	uint8_t inputPort;
	if (i2c_read(pm342->i2c, TCA9539_A0_L_A1_L_ADDR, TCA9539_INPUT_PORT_1_REG, &inputPort))
		return 1;

	*vdd_core_rail = ~inputPort & TCA9539_P16_FLAG;
	*vdd_cpu_rail = ~inputPort & TCA9539_P17_FLAG;

	return 0;
}
