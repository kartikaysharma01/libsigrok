/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2021 Bhaskar Sharma <bhaskar.sharma@groww.in>
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

static const char *analog_channels[] = {
        "CH1",
        "CH2",
        "CH3",
        "MIC",
        "CAP",
        "RES",
        "VOL",
        "AN4",
};


static struct sr_dev_driver pslab_driver_info;

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
    (void) options;
    GSList *l, *devices;
    // V5
    GSList *device_paths = g_slist_append(sr_serial_find_usb(0x04D8,0x00DF), sr_serial_find_usb(0x10C4,0xEA60));
    //V6
//    GSList *v6_device_paths = sr_serial_find_usb(0x10C4,0xEA60);

    devices = NULL;
    struct sr_serial_dev_inst *serial;
    struct sr_dev_inst *sdi;

    if(device_paths) {
        for(l = device_paths; l; l = l->next) {

            serial = sr_serial_dev_inst_new(l->data, NULL);

            sdi = g_new0(struct sr_dev_inst, 1);
            sdi->status = SR_ST_INACTIVE;
            sdi->inst_type = SR_INST_SERIAL;
            sdi->vendor = g_strdup("test");
            sdi->model = g_strdup("V5");
            sdi->version = g_strdup("version"); // send command to device and retrive
            sdi->connection_id = l->data; // make a unique conn id to identify devices -- eg. port_name
            sdi->conn = serial;

            struct sr_channel_group *cg = g_malloc(sizeof(struct sr_channel_group));
            cg->name = g_strdup("Analog");
            cg->channels = g_slist_alloc();
            for(int i=0; i<NUM_ANALOG_CHANNELS; i++) {
                struct sr_channel *ch = sr_channel_new(sdi, i, SR_CHANNEL_ANALOG, TRUE, analog_channels[i]);
                cg->channels = g_slist_append(cg->channels, ch);
            }
            sdi->channel_groups = g_slist_append(NULL, cg);
            devices = g_slist_append(devices, sdi);
        }
    }

    /* TODO: scan for devices, either based on a SR_CONF_CONN option
	 * or on a USB scan. */

	return std_scan_complete(di,  devices);
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

static struct sr_dev_driver pslab_driver_info = {
	.name = "pslab",
	.longname = "pslab",
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
