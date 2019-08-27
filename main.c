#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "pm342.h"

char *pm342_product, *pm342_serial, *pm342_index;

int list_devices()
{
	const char **descriptions;
	const char **serials;
	unsigned int numDevices;
	if (pm342_get_devices(&descriptions, &serials, &numDevices))
		return 1;

	int index = -1;
	puts("PM342_INDEX PM342_SERIAL		 PM342_PRODUCT");
	for (int ii = 0; ii < numDevices; ++ii)
	{
		if ((!pm342_product || !strcmp(pm342_product, descriptions[ii])) && (!pm342_serial || !strcmp(pm342_serial, serials[ii])))
		{
			++index;
			if (!pm342_index || index == atoi(pm342_index))
				printf("%-11d %-16s %s\n", index, serials[ii], descriptions[ii]);
		}
	}
	free(descriptions);
	free(serials);
	return 0;
}

int main(int argc, const char **argv, const char **envp)
{
	char * p = getenv("PM342_PRODUCT");
	if (p)
		pm342_product = p;

	p = getenv("PM342_SERIAL");
	if (p)
		pm342_serial = p;

	p = getenv("PM342_INDEX");
	if (p)
		pm342_index = p;

	if (argc != 2)
	{
		fprintf(stderr, "%s [reset|reset_down|reset_up|onkey|onkey_down|onkey_up|onkey_hold|onkey_reset_recovery|reset_recovery|recovery_down|recovery_up|force_off|force_off_down|force_off_up|usb_off|usb_on|status|list]\n", argv[0]);
		return 1;
	}

	if (!strcmp(argv[1], "list"))
	{
		if (list_devices())
		{
			fprintf(stderr, "An unknown error occurred!\n");
			return 1;
		}
		
		return 0;
	}

	pm342_context *pm342 = pm342_new(pm342_product, pm342_serial, pm342_index ? atoi(pm342_index) : 0);
	if (!pm342)
	{
		fprintf(stderr, "Cannot open pm342 device.	Either need root access or no devices found.\n");
		return 1;
	}

	if (!strcmp(argv[1], "reset"))
	{
		if (pm342_toggle_keys(pm342, PM342PK_RESET, PM342PK_RESET, 100000))
			goto err;
	}

	else if (!strcmp(argv[1], "onkey"))
	{
		if (pm342_toggle_keys(pm342, PM342PK_ONKEY, PM342PK_ONKEY, 100000))
			goto err;
	}

	else if (!strcmp(argv[1], "onkey_hold"))
	{
		if (pm342_toggle_keys(pm342, PM342PK_ONKEY, PM342PK_ONKEY, 12000000))
			goto err;
	}

	else if (!strcmp(argv[1], "onkey_down"))
	{
		if (pm342_set_keys(pm342, PM342PK_ONKEY, PM342PK_ONKEY))
			goto err;
	}

	else if (!strcmp(argv[1], "onkey_up"))
	{
		if (pm342_set_keys(pm342, PM342PK_ONKEY, 0))
			goto err;
	}

	else if (!strcmp(argv[1], "reset_down"))
	{
		if (pm342_set_keys(pm342, PM342PK_RESET, PM342PK_RESET))
			goto err;
	}

	else if (!strcmp(argv[1], "reset_up"))
	{
		if ( pm342_set_keys(pm342, PM342PK_RESET, 0) )
			goto err;
	}

	else if (!strcmp(argv[1], "onkey_reset_recovery"))
	{
		if (pm342_set_keys(pm342, PM342PK_FORCE_RECOVERY, PM342PK_FORCE_RECOVERY))
			goto err;
		usleep(100000);

		if (pm342_set_keys(pm342, PM342PK_RESET, PM342PK_RESET))
			goto err;
		usleep(100000);

		if (pm342_set_keys(pm342, PM342PK_ONKEY, PM342PK_ONKEY))
			goto err;
		usleep(5000000);

		if (pm342_set_keys(pm342, PM342PK_RESET, 0))
			goto err;
		usleep(100000);

		if (pm342_set_keys(pm342, PM342PK_ONKEY, 0))
			goto err;
		usleep(100000);

		if (pm342_set_keys(pm342, PM342PK_FORCE_RECOVERY, 0))
			goto err;
	}

	else if (!strcmp(argv[1], "reset_recovery"))
	{
		pm342_set_keys(pm342, PM342PK_FORCE_RECOVERY, PM342PK_FORCE_RECOVERY);
		usleep(100000);

		if (pm342_toggle_keys(pm342, PM342PK_RESET, PM342PK_RESET, 100000))
			goto err;
		usleep(100000);

		if (pm342_set_keys(pm342, PM342PK_FORCE_RECOVERY, 0))
			goto err;
	}

	else if (!strcmp(argv[1], "recovery_down"))
	{
		if (pm342_set_keys(pm342, PM342PK_FORCE_RECOVERY, PM342PK_FORCE_RECOVERY))
			goto err;
	}

	else if (!strcmp(argv[1], "recovery_up"))
	{
		if (pm342_set_keys(pm342, PM342PK_FORCE_RECOVERY, 0))
			goto err;
	}

	else if (!strcmp(argv[1], "force_off"))
	{
		if (pm342_toggle_keys(pm342, PM342PK_FORCE_OFF, PM342PK_FORCE_OFF, 100000))
			goto err;
	}

	else if (!strcmp(argv[1], "force_off_down"))
	{
		if (pm342_set_keys(pm342, PM342PK_FORCE_OFF, PM342PK_FORCE_OFF))
			goto err;
	}

	else if (!strcmp(argv[1], "force_off_up"))
	{
		if (pm342_set_keys(pm342, PM342PK_FORCE_OFF, 0))
			goto err;
	}

	else if (!strcmp(argv[1], "usb_off"))
	{
		if (pm342_set_keys(pm342, PM342PK_USB, 0))
			goto err;
	}

	else if (!strcmp(argv[1], "usb_on"))
	{
		if (pm342_set_keys(pm342, PM342PK_USB, PM342PK_USB))
			goto err;
	}

	else if (!strcmp(argv[1], "status"))
	{
		int vdd_core_rail, vdd_cpu_rail;
		if (pm342_get_status(pm342, &vdd_core_rail, &vdd_cpu_rail))
			goto err;

		printf("Core Rail: %s\nCPU Rail: %s\n", vdd_core_rail ? "ON" : "OFF", vdd_cpu_rail ? "ON" : "OFF");
	}
	else
	{
		fprintf(stderr, "Unknown argument!\n");
		if (pm342)
		{
			pm342_free(pm342);
			pm342 = 0;
		}
		return 1;
	}

	pm342_free(pm342);
	pm342 = 0;
	return 0;

err:
	fprintf(stderr, "An unknown error occurred!\n");

	if (pm342)
	{
		pm342_free(pm342);
		pm342 = 0;
	}
	return 1;
}