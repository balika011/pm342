#ifndef __PM342_H__
#define __PM342_H__

#include <stdint.h>
#include <stdlib.h>

#include "i2c.h"

typedef struct 
{
	i2c_context *i2c;
} pm342_context;

typedef enum : uint32_t
{
	PM342PK_RESET = 0x1,
	PM342PK_ONKEY = 0x2,
	PM342PK_FORCE_RECOVERY = 0x4,
	PM342PK_FORCE_OFF = 0x8,
	PM342PK_USB = 0x10,
	PM342PK_COUNT = 0x11,
} PM342PhidgetKeys;

int pm342_get_devices(const char ***descriptions, const char ***serials, unsigned int *num_devices);
pm342_context *pm342_new(char *product, char *serial, unsigned int index);
void pm342_free(pm342_context *pm342);
int pm342_toggle_keys(pm342_context *pm342, PM342PhidgetKeys keys, int state, int timeout);
int pm342_set_keys(pm342_context *pm342, PM342PhidgetKeys keys, int state);
int pm342_get_status(pm342_context *pm342, int *vdd_core_rail, int *vdd_cpu_rail);

#endif