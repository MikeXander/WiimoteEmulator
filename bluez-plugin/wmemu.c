//#ifdef HAVE_CONFIG_H  // I needed to comment this out
//#include <config.h>   // because it tried to include
//#endif                // the config no matter what

#include <stdbool.h>

#include <bluetooth/bluetooth.h>
#include <glib.h>

#include "src/plugin.h"
#include "src/adapter.h"
#include "src/device.h"
#include "src/log.h"

static ssize_t wmemu_pincb(struct btd_adapter *adapter, struct btd_device *device,
						char *pinbuf, gboolean *display)
{
	//force Wii pin (bdaddr bytes backwards)
	//very basic, no filtering, does this for all devices
	memcpy(pinbuf, device, 6);
	return 6;
}

static int wmemu_probe(struct btd_adapter *adapter)
{
	btd_adapter_register_pin_cb(adapter, wmemu_pincb);

	return 0;
}

static void wmemu_remove(struct btd_adapter *adapter)
{
	btd_adapter_unregister_pin_cb(adapter, wmemu_pincb);
}

static struct btd_adapter_driver wmemu_driver = {
	.name	= "wmemu",
	.probe	= wmemu_probe,
	.remove	= wmemu_remove,
};

static int wmemu_init(void)
{
	return btd_register_adapter_driver(&wmemu_driver);
}

static void wmemu_exit(void)
{
	btd_unregister_adapter_driver(&wmemu_driver);
}

BLUETOOTH_PLUGIN_DEFINE(wmemu, __VERSION__,
		BLUETOOTH_PLUGIN_PRIORITY_LOW, wmemu_init, wmemu_exit)
