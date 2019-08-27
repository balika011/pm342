#ifndef __MPSSE_H__
#define __MPSSE_H__

#include <stdint.h>
#include <ftdi.h>

typedef struct 
{
	ftdi_context ftdi;
	uint8_t buf[1024];
	unsigned int len;
} mpsse_context;

int mpsse_get_devices(int vid, int pid, const char ***descriptions, const char ***serials, unsigned int *num_devices);
mpsse_context *mpsse_new(int vid, int pid, const char *description, const char *serial, unsigned int index, ftdi_interface interface);
void mpsse_free(mpsse_context *mpsse);

void mpsse_queue(mpsse_context *mpsse, uint8_t *buf, unsigned int len);
void mpsse_flush(mpsse_context *mpsse);
int mpsse_raw_read(mpsse_context *mpsse, uint8_t *data, unsigned int len);

#endif