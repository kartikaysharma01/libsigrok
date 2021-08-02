/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2021 Kartikay Sharma <sharma.kartik2107@gmail.com>
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
#include "protocol.h"

static struct sr_dev_driver pslab_logic_analyzer_driver_info;

static const uint32_t scanopts[] = {
	SR_CONF_CONN,
	SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
	SR_CONF_LOGIC_ANALYZER,
};

static const uint32_t devopts[] = {
	SR_CONF_LIMIT_SAMPLES | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_LIMIT_MSEC | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_SAMPLE_INTERVAL | SR_CONF_GET | SR_CONF_SET,
	SR_CONF_TRIGGER_SOURCE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_TRIGGER_PATTERN | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
};

static const uint32_t devopts_cg[] = {
	SR_CONF_PATTERN_MODE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
};

static const char *digital_channels[] = {
	"LA1",
	"LA2",
	"LA3",
	"LA4",
	"RES",
	"EXT",
	"FRQ",
};

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	GSList *l, *devices;
	struct sr_config *src;
	struct sr_serial_dev_inst *serial;
	struct sr_dev_inst *sdi;
	struct dev_context *devc;
	struct sr_channel *ch;
	struct channel_priv *cp;
	struct sr_channel_group *cg;
	struct channel_group_priv *cgp;
	const char *path, *serialcomm;
	char *device_path, *version;
	int i;

	GSList *device_paths_v5 = sr_serial_find_usb(0x04D8, 0x00DF);

	GSList *device_paths_v6 = sr_serial_find_usb(0x10C4, 0xEA60);

	GSList *device_paths = g_slist_concat(device_paths_v5, device_paths_v6);

	devices = NULL;
	path = NULL;
	serialcomm = "1000000/8n1";

	for (l = options; l; l = l->next) {
		src = l->data;
		switch (src->key) {
		case SR_CONF_CONN:
			path = g_variant_get_string(src->data, NULL);
			break;
		case SR_CONF_SERIALCOMM:
			serialcomm = g_variant_get_string(src->data, NULL);
			break;
		}
	}

	for (l = device_paths; l; l = l->next) {
		device_path = l->data;
		if (path && path != device_path)
			continue;

		serial = sr_serial_dev_inst_new(device_path, serialcomm);

		if (serial_open(serial, SERIAL_RDWR) != SR_OK)
			continue;

		version = pslab_get_version(serial);
		gboolean isPSLabDevice = g_str_has_prefix(version, "PSLab") ||
					 g_str_has_prefix(version, "CSpark");
		if (!isPSLabDevice) {
			g_free(version);
			serial_close(serial);
			continue;
		}
		sr_info("PSLab device found: %s on port: %s", version, device_path);

		sdi = g_new0(struct sr_dev_inst, 1);
		devc = g_new0(struct dev_context, 1);
		sdi->status = SR_ST_INACTIVE;
		sdi->inst_type = SR_INST_SERIAL;
		sdi->vendor = g_strdup("FOSSASIA");
		sdi->connection_id = device_path;
		sdi->conn = serial;
		sdi->version = g_strdup(version);

		for (i = 0; i < NUM_DIGITAL_INPUT_CHANNEL; i++) {
			ch = sr_channel_new(sdi, i, SR_CHANNEL_LOGIC, TRUE, digital_channels[i]);
			cp = g_new0(struct channel_priv, 1);
			cg = g_new0(struct sr_channel_group, 1);
			cgp = g_new0(struct channel_group_priv, 1);
			cp->events_in_buffer = 0;
			cp->datatype = g_strdup("long");
			if (!g_strcmp0(digital_channels[i], "LA1"))
				devc->channel_one_map = ch;
			else if (!g_strcmp0(digital_channels[i], "LA2"))
				devc->channel_two_map = ch;
			cg->name = g_strdup(digital_channels[i]);
			cg->channels = g_slist_append(NULL, ch);
			cgp->logic_mode = LOGIC_MODES[0];
			cg->priv = cgp;
			sdi->channel_groups = g_slist_append(sdi->channel_groups, cg);
		}
		sr_sw_limits_init(&devc->limits);
		devc->limits.limit_samples = 2500;
		devc->limits.limit_msec = 1000;
		devc->trigger_enabled = FALSE;
		devc->trigger_pattern = g_strdup("disabled");
		devc->trigger_channel = *devc->channel_one_map;
		devc->prescaler = 0;
		devc->trimmed = 0;
		sdi->priv = devc;

		devices = g_slist_append(devices, sdi);
		g_free(version);
		serial_close(serial);
	}
	return std_scan_complete(di, devices);
}

static int dev_open(struct sr_dev_inst *sdi)
{
	(void)sdi;

	/* TODO: get handle from sdi->conn and open it. */

	return SR_OK;
}

static int dev_close(struct sr_dev_inst *sdi)
{
	(void)sdi;

	/* TODO: get handle from sdi->conn and close it. */

	return SR_OK;
}

static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		ret = SR_ERR_NA;
	}

	return ret;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
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

static struct sr_dev_driver pslab_logic_analyzer_driver_info = {
	.name = "pslab-logic-analyzer",
	.longname = "PSLab Logic Analyzer",
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
SR_REGISTER_DEV_DRIVER(pslab_logic_analyzer_driver_info);
