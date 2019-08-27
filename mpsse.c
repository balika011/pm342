#include <stdio.h>
#include "mpsse.h"

int mpsse_get_devices(int vid, int pid, const char ***descriptions, const char ***serials, unsigned int *num_devices)
{
	ftdi_context ftdi;
	int status = ftdi_init(&ftdi);
	if (status)
		return 1;

	ftdi_device_list *deviceList = 0;
	char **descStrings = 0;
	char **serialStrings = 0;

	int numDevices = ftdi_usb_find_all(&ftdi, &deviceList, vid, pid);
	if (numDevices >= 0)
	{
		descStrings = (char **) malloc(264 * numDevices);
		serialStrings = (char **) malloc(264 * numDevices);
		ftdi_device_list *devicePtr = deviceList;
		for (int ii = 0; ii < numDevices; ++ii)
		{
			descStrings[ii] = (char *)&descStrings[numDevices] + (unsigned int)(ii << 8);
			serialStrings[ii] = (char *)&serialStrings[numDevices] + (unsigned int)(ii << 8);
			if (!devicePtr || ftdi_usb_get_strings(&ftdi, devicePtr->dev, 0, 0, descStrings[ii], 256, serialStrings[ii], 256))
			{
				status = 1;
				break;
			}
			devicePtr = devicePtr->next;
		}
		if (devicePtr)
			status = 1;
	}
	else
		status = 1;

	if (status)
	{
		free(descStrings);
		free(serialStrings);
		*descriptions = 0;
		*serials = 0;
		*num_devices = 0;
	}
	else
	{
		*descriptions = (const char **) descStrings;
		*serials = (const char **) serialStrings;
		*num_devices = numDevices;
	}

	if (deviceList)
		ftdi_list_free(&deviceList);
	ftdi_deinit(&ftdi);

	return status;
}

int ftdi_sync(ftdi_context *ftdi)
{
	uint8_t buf[2];
	buf[0] = 0xAA;
	ftdi_write_data(ftdi, buf, 1LL);

	for (int i = 0; ; ++i)
	{
		if (i > 4)
			return 1;

		int count = ftdi_read_data(ftdi, buf, 2LL);
		if (count < 0)
			return 1;

		if (count == 2)
			break;
	}

	return buf[0] != 0xFA || buf[1] != 0xAA;
}

mpsse_context *mpsse_new(int vid, int pid, const char *description, const char *serial, unsigned int index, ftdi_interface interface)
{
	int isInit = 0;
	int isOpen = 0;
	mpsse_context *mpsse = (mpsse_context *) malloc(sizeof(mpsse_context));
	if (mpsse)
	{
		if (ftdi_init(&mpsse->ftdi) >= 0)
		{
			isInit = 1;
			if (ftdi_set_interface(&mpsse->ftdi, interface) >= 0)
			{
				int status = ftdi_usb_open_desc_index(&mpsse->ftdi, vid, pid, description, serial, index);
				if (status >= 0)
				{
					isOpen = 1;
					status |= ftdi_usb_reset(&mpsse->ftdi) | status;
					status | ftdi_usb_purge_rx_buffer(&mpsse->ftdi) ;
					status |= ftdi_usb_purge_tx_buffer(&mpsse->ftdi);
					status |= ftdi_set_bitmode(&mpsse->ftdi, 255, 2);
					if (ftdi_sync(&mpsse->ftdi) | status >= 0)
						return mpsse;
				}
			}
		}
	}

	if (isOpen)
		ftdi_usb_close(&mpsse->ftdi);

	if (isInit)
		ftdi_deinit(&mpsse->ftdi);

	if (mpsse)
		free(mpsse);

	return 0;
}

void mpsse_free(mpsse_context *mpsse)
{
	ftdi_usb_close(&mpsse->ftdi);
	ftdi_deinit(&mpsse->ftdi);
	free(mpsse);
}

void mpsse_queue(mpsse_context *mpsse, uint8_t *buf, unsigned int len)
{
	for (int i = 0; i < len; ++i)
	{
		mpsse->buf[mpsse->len] = buf[i];
		++mpsse->len;
	}
}

void mpsse_flush(mpsse_context *mpsse)
{
	if (ftdi_write_data(&mpsse->ftdi, mpsse->buf, mpsse->len) != mpsse->len)
		fprintf(stderr, "ftdi_write_data(&mpsse->ftdi, mpsse->buf, mpsse->len) == mpsse->len");
	mpsse->len = 0;
}

int mpsse_raw_read(mpsse_context *mpsse, uint8_t *data, unsigned int len)
{
	return ftdi_read_data(&mpsse->ftdi, data, len);
}
