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

static const uint32_t scanopts[] = {
	SR_CONF_CONN,
	SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
	SR_CONF_SIGNAL_GENERATOR,
};

static const uint32_t devopts[] = {
	SR_CONF_CONTINUOUS,
	SR_CONF_OUTPUT_FREQUENCY | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
};

static const uint32_t devopts_cg[] = {
	SR_CONF_DUTY_CYCLE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
	SR_CONF_PHASE | SR_CONF_GET | SR_CONF_SET | SR_CONF_LIST,
};

static const struct digital_output_channel digital_output[] = {
	{"SQ1", 0x10},
	{"SQ2", 0x20},
	{"SQ3", 0x40},
	{"SQ4", 0x80},
};

static const double output_freq_min_max_step[] = { 10, 10000000, 1 };

static const double phase_min_max_step[] = { 0.0, 360.0, 0.001 };

static const double duty_cycle_min_max_step[] = { 0.0, 100.0, 0.001 };

static struct sr_dev_driver pslab_pwm_generator_driver_info;

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	GSList *l, *devices;
	struct sr_config *src;
	struct sr_serial_dev_inst *serial;
	struct sr_dev_inst *sdi;
	struct dev_context *devc;
	struct sr_channel *ch;
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
		devc->frequency = 0;
		sdi->priv = devc;

		for (i = 0; i < NUM_DIGITAL_OUTPUT_CHANNEL; i++) {
			ch = sr_channel_new(sdi, i, SR_CHANNEL_LOGIC,
					     TRUE, digital_output[i].name);
			cg = g_new0(struct sr_channel_group, 1);
			cgp = g_new0(struct channel_group_priv, 1);
			cg->name = g_strdup(digital_output[i].name);
			cg->channels = g_slist_append(NULL, ch);
			cgp->duty_cycle = 0;
			cgp->phase = 0;
			cgp->state = g_strdup("LOW");
			cgp->state_mask = digital_output[i].state_mask;
			cg->priv = cgp;
			sdi->channel_groups = g_slist_append(sdi->channel_groups, cg);
		}

		devices = g_slist_append(devices, sdi);
		serial_close(serial);
	}
	return std_scan_complete(di, devices);
}

static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;
	struct channel_group_priv *cp;

	if (!sdi)
		return SR_ERR_ARG;

	devc = sdi->priv;

	if(!cg) {
		switch (key) {
		case SR_CONF_OUTPUT_FREQUENCY:
			*data = g_variant_new_double(devc->frequency);
			break;
		default:
			return SR_ERR_NA;
		}
	} else {
		switch (key) {
		case SR_CONF_DUTY_CYCLE:
			cp = cg->priv;
			*data = g_variant_new_double(cp->duty_cycle * 100);
			break;
		case SR_CONF_PHASE:
			cp = cg->priv;
			*data = g_variant_new_double(cp->phase * 360);
			break;
		default:
			return SR_ERR_NA;
		}
	}

	return SR_OK;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	struct dev_context *devc;
	struct channel_group_priv *cp;
	double tmp;

	if (!sdi)
		return SR_ERR_ARG;

	devc = sdi->priv;

	if (!cg) {
		switch (key) {
		case SR_CONF_OUTPUT_FREQUENCY:
			devc->frequency = g_variant_get_double(data);
			break;
		default:
			return SR_ERR_NA;
		}
	} else {
		switch (key) {
		case SR_CONF_DUTY_CYCLE:
			cp = cg->priv;
			tmp = g_variant_get_double(data);
			tmp = tmp / 100;
			cp->duty_cycle = tmp;

			// set state
			if (cp->duty_cycle == 0) {
				cp->state = g_strdup("LOW");
			} else if (cp->duty_cycle < 1) {
				cp->state = g_strdup("PWM");
			} else if (cp->duty_cycle == 1) {
				cp->state = g_strdup("HIGH");
			} else {
				sr_err("Duty Cycle can not be greater than 100");
				return SR_ERR_ARG;
			}
			break;
		case SR_CONF_PHASE:
			cp = cg->priv;
			tmp = g_variant_get_double(data);
			tmp = tmp / 360;
			cp->phase = tmp;
			break;
		default:
			return SR_ERR_NA;
		}
	}

	return SR_OK;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg) {
	if (!cg) {
		switch (key) {
		case SR_CONF_DEVICE_OPTIONS:
		case SR_CONF_SCAN_OPTIONS:
			return STD_CONFIG_LIST(key, data, sdi, cg, scanopts, drvopts, devopts);
		case SR_CONF_OUTPUT_FREQUENCY:
			*data = std_gvar_min_max_step_array(output_freq_min_max_step);
			break;
		default:
			return SR_ERR_NA;
		}
	} else {
		switch (key) {
		case SR_CONF_DEVICE_OPTIONS:
			*data = std_gvar_array_u32(ARRAY_AND_SIZE(devopts_cg));
			break;
		case SR_CONF_PHASE:
			*data = std_gvar_min_max_step_array(phase_min_max_step);
			break;
		case SR_CONF_DUTY_CYCLE:
			*data = std_gvar_min_max_step_array(duty_cycle_min_max_step);
			break;
		default:
			return SR_ERR_NA;
		}
	}

	return SR_OK;
}

static void configure_channels(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	const GSList *l;
	struct sr_channel *ch;

	devc = sdi->priv;

	g_slist_free(devc->enabled_digital_output);
	devc->enabled_digital_output = NULL;

	for (l = sdi->channels; l; l = l->next) {
		ch = l->data;
		if (ch->enabled) {
			devc->enabled_digital_output =
				g_slist_append(devc->enabled_digital_output, ch);
			sr_info("enabled channels: {} %s", ch->name);
		}
	}
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	struct sr_serial_dev_inst *serial;

	if (!sdi)
		return SR_ERR_ARG;

	devc = sdi->priv;
	serial = sdi->conn;

	configure_channels(sdi);
	if (!devc->enabled_digital_output) {
		sr_err("No channels enabled");
		return SR_ERR_ARG;
	}

	if (devc->frequency > HIGH_FREQUENCY_LIMIT || devc->frequency <= 0) {
		sr_err("Frequency should be greater than 0 and less than 10 MHz");
		return SR_ERR_ARG;
	}

	std_session_send_df_header(sdi);

	serial_source_add(sdi->session, serial, G_IO_IN, 10,
			  pslab_pwm_generator_receive_data, (void *)sdi);

	return SR_OK;
}

static struct sr_dev_driver pslab_pwm_generator_driver_info = {
	.name = "pslab-pwm-generator",
	.longname = "PSLab PWM generator",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = std_dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = std_serial_dev_open,
	.dev_close = std_serial_dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = std_serial_dev_acquisition_stop,
	.context = NULL,
};
SR_REGISTER_DEV_DRIVER(pslab_pwm_generator_driver_info);
