/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2021 Karikay Sharma <sharma.kartik2107@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <math.h>
#include "protocol.h"

static const uint32_t scanopts[] = {
		SR_CONF_CONN,
};

static const uint32_t drvopts[] = {
		SR_CONF_OSCILLOSCOPE,
		SR_CONF_MULTIMETER,
};

static const uint32_t devopts[] = {
		SR_CONF_CONTINUOUS | SR_CONF_SET,
		SR_CONF_LIMIT_SAMPLES | SR_CONF_GET | SR_CONF_SET,
		SR_CONF_SPL_WEIGHT_FREQ | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
		SR_CONF_SPL_WEIGHT_TIME | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
		SR_CONF_SPL_MEASUREMENT_RANGE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
		SR_CONF_POWER_OFF | SR_CONF_GET | SR_CONF_SET,
		SR_CONF_DATA_SOURCE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
};

static const struct analog_channel analog_channels[] = {
		{"CH1", 3},
		{"CH2", 0},
		{"CH3", 1},
		{"MIC", 2},
		{"AN4", 4},
		{"RES", 7},
		{"CAP", 5},
		{"VOL", 8},
};


static struct sr_dev_driver pslab_driver_info;

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	GSList *l, *devices;
	struct sr_config *src;

	GSList *device_paths_v5 = sr_serial_find_usb(0x04D8, 0x00DF);

	GSList *device_paths_v6 = sr_serial_find_usb(0x10C4, 0xEA60);

	GSList *device_paths = g_slist_concat(device_paths_v5, device_paths_v6);

	devices = NULL;
	struct sr_serial_dev_inst *serial;
	struct sr_dev_inst *sdi;

	const char *path;
	const char *serialcomm = "1000000/8n1";

	for (l = options; l; l = l->next) {
		src = l->data;
		switch (src->key) {
			case SR_CONF_CONN:
				path = g_variant_get_string(src->data, NULL);
				device_paths = g_slist_append(NULL, path);
				break;
			case SR_CONF_SERIALCOMM:
				serialcomm = g_variant_get_string(src->data, NULL);
				break;
		}
	}
	for (l = device_paths; l; l = l->next)
	{
		serial = sr_serial_dev_inst_new(l->data, serialcomm);

		sdi = g_new0(struct sr_dev_inst, 1);
		sdi->status = SR_ST_INACTIVE;
		sdi->inst_type = SR_INST_SERIAL;
		sdi->vendor = g_strdup("FOSSASIA");
		sdi->connection_id = l->data;
		sdi->conn = serial;

		struct sr_channel_group *cg = g_new0(struct sr_channel_group, 1);
		cg->name = g_strdup("Analog");
		cg->channels = g_slist_alloc();
		for (int i = 0; i < NUM_ANALOG_CHANNELS; i++)
		{
			struct sr_channel *ch = sr_channel_new(sdi, i, SR_CHANNEL_ANALOG, TRUE, analog_channels[i].name);
			struct channel_priv *cp = g_new0(struct channel_priv, 1);
			cp->chosa = analog_channels[i].chosa;
			cp->gain = 1;
			cp->resolution = pow(2, 10) - 1;
			if (!g_strcmp0(analog_channels[i].name, "CH1"))
			{
				cp->programmable_gain_amplifier = 1;
			}
			else if (!g_strcmp0(analog_channels[i].name, "CH2"))
			{
				cp->programmable_gain_amplifier = 2;
			}

			ch->priv = cp;
			cg->channels = g_slist_append(cg->channels, ch);
		}
		sdi->channel_groups = g_slist_append(NULL, cg);
		devices = g_slist_append(devices, sdi);
	}
	return std_scan_complete(di, devices);
}

static int dev_open(struct sr_dev_inst *sdi)
{
	struct sr_serial_dev_inst *serial = sr_serial_dev_inst_new(sdi->connection_id, "1000000/8n1");
	return serial_open(serial, SERIAL_RDWR);
}

static int dev_close(struct sr_dev_inst *sdi)
{
	return serial_close(sdi->conn);
}

static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)cg;
	*data = g_variant_new_uint64(100000);
	return SR_OK;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	return SR_OK;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	switch (key) {
		case SR_CONF_SCAN_OPTIONS:
		case SR_CONF_DEVICE_OPTIONS:
			return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
		default:
			return SR_ERR_NA;
	}

	return SR_OK;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	/* TODO: configure hardware, reset acquisition state, set up
	 * callbacks and send header packet. */

	(void)sdi;

	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	/* TODO: stop acquisition. */

	(void)sdi;

	return SR_OK;
}

static struct sr_dev_driver pslab_driver_info = {
	.name = "pslab",
	.longname = "PSLab",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = dev_open,
	.dev_close = dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,
	.context = NULL,
};
SR_REGISTER_DEV_DRIVER(pslab_driver_info);
