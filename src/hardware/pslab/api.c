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
#include "protocol.h"

static const uint32_t scanopts[] = {
        SR_CONF_CONN,
        SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
        SR_CONF_OSCILLOSCOPE,
};

static const uint32_t devopts[] = {
        SR_CONF_LIMIT_FRAMES | SR_CONF_GET | SR_CONF_SET,
        SR_CONF_SAMPLERATE | SR_CONF_GET,
};

static struct sr_dev_driver pslab_driver_info;

static const uint8_t GET_VERSION[] = { 0xb, 0x5 };

static const struct pslab_profile supported_device[] = {
        /* V5 */
        { 0x04D8, 0x00DF, "PSLab", "V5", NULL,
          "dreamsourcelab-dslogic-fx2.fw",
          0, "DreamSourceLab", "DSLogic", 256 * 1024 * 1024},
        /* V6 */
        { 0x10C4, 0xEA60, "PSLab", "v6", NULL,
          "dreamsourcelab-dscope-fx2.fw",
          0, "DreamSourceLab", "DSCope", 256 * 1024 * 1024},

        ALL_ZERO
};


static gboolean is_plausible(const struct libusb_device_descriptor *des)
{
    for (int i = 0; supported_device[i].vid; i++) {
        if (des->idVendor != supported_device[i].vid)
            continue;
        if (des->idProduct == supported_device[i].pid)
            return TRUE;
    }

    return FALSE;
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	struct drv_context *drvc;
    struct dev_context *devc;
    struct sr_dev_inst *sdi;
    struct sr_usb_dev_inst *usb;
    struct sr_channel *ch;
    struct sr_channel_group *cg;
    struct sr_config *src;
    const struct pslab_profile *prof;
    GSList *l, *devices, *conn_devices;
    struct libusb_device_descriptor des;
    libusb_device **devlist;
    struct libusb_device_handle *hdl;
    int ret, i, j;
    const char *conn;
    char manufacturer[64], connection_id[64];

	drvc = di->context;
	conn = NULL;

    for (l = options; l; l = l->next) {
        src = l->data;
        switch (src->key) {
            case SR_CONF_CONN:
                conn = g_variant_get_string(src->data, NULL);
                break;
        }
    }

    if (conn)
        conn_devices = sr_usb_find(drvc->sr_ctx->libusb_ctx, conn);
    else
        conn_devices = NULL;

    devices = NULL;
    libusb_get_device_list(drvc->sr_ctx->libusb_ctx, &devlist);

    for (i = 0; devlist[i]; i++) {
        if (conn) {
            usb = NULL;
            for (l = conn_devices; l; l = l->next) {
                usb = l->data;
                if (usb->bus == libusb_get_bus_number(devlist[i])
                    && usb->address == libusb_get_device_address(devlist[i]))
                    break;
            }
            if (!l)
                /* This device matched none of the ones that
                 * matched the conn specification. */
                continue;
        }

        libusb_get_device_descriptor(devlist[i], &des);

        if (!is_plausible(&des))
            continue;

        if ((ret = libusb_open(devlist[i], &hdl)) < 0) {
            sr_warn("Failed to open potential device with "
                    "VID:PID %04x:%04x: %s.", des.idVendor,
                    des.idProduct, libusb_error_name(ret));
            continue;
        }

        if (des.iManufacturer == 0) {
            manufacturer[0] = '\0';
        } else if ((ret = libusb_get_string_descriptor_ascii(hdl, des.iManufacturer, (unsigned char *) manufacturer,
                                                             sizeof(manufacturer))) < 0) {
            sr_warn("Failed to get manufacturer string descriptor: %s.", libusb_error_name(ret));
            continue;
        }

        libusb_close(hdl);

        if (usb_get_port_path(devlist[i], connection_id, sizeof(connection_id)) < 0)
            continue;

        /* Unknown iManufacturer string, ignore. */
        if (strcmp(manufacturer, "PSLab") && strcmp(manufacturer, "CSpark"))
            continue;

        prof = NULL;
        for (j = 0; supported_device[j].vid; j++) {
            if (des.idVendor == supported_device[j].vid && des.idProduct == supported_device[j].pid) {
                prof = &supported_device[j];
                break;
            }
        }

        if (!prof)
            continue;

        sdi = g_malloc0(sizeof(struct sr_dev_inst));
        sdi->vendor = g_strdup(prof->vendor);
        sdi->model = g_strdup(prof->model);
        sdi->version = g_strdup(prof->model_version);
        sdi->connection_id = g_strdup(connection_id);

        devices = g_slist_append(devices, sdi);

        sr_dbg("Found device with version %d (%04x:%04x) %s.", des.iManufacturer,
               des.idVendor, des.idProduct, connection_id);

        sdi->status = SR_ST_INACTIVE;
        sdi->inst_type = SR_INST_USB; /*SR_INST_SERIAL (?) */
        sdi->conn = sr_usb_dev_inst_new(libusb_get_bus_number(devlist[i]),
                                        libusb_get_device_address(devlist[i]), NULL);
    }

    libusb_free_device_list(devlist, 1);
    g_slist_free_full(conn_devices, (GDestroyNotify)sr_usb_dev_inst_free);

    return std_scan_complete(di, devices);
}

static int dev_open(struct sr_dev_inst *sdi)
{
    struct sr_dev_driver *di = sdi->driver;
    struct sr_usb_dev_inst *usb;
    struct dev_context *devc;
    int ret;
//    int64_t timediff_us, timediff_ms;

    devc = sdi->priv;
    usb = sdi->conn;

    ret = libusb_claim_interface(usb->devhdl, USB_INTERFACE);
    if (ret != 0) {
        switch (ret) {
            case LIBUSB_ERROR_BUSY:
                sr_err("Unable to claim USB interface. Another "
                       "program or driver has already claimed it.");
                break;
            case LIBUSB_ERROR_NO_DEVICE:
                sr_err("Device has been disconnected.");
                break;
            default:
                sr_err("Unable to claim interface: %s.",
                       libusb_error_name(ret));
                break;
        }

        return SR_ERR;
    }

    if (devc->cur_samplerate == 0) {
        /* Samplerate hasn't been set; default to the slowest one. */
        devc->cur_samplerate = devc->samplerates[0];
    }

    if (devc->cur_threshold == 0.0) {
        devc->cur_threshold = thresholds[1][0];
        return dslogic_set_voltage_threshold(sdi, devc->cur_threshold);
    }

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
