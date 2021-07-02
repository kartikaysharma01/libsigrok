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

#ifndef LIBSIGROK_HARDWARE_PSLAB_PROTOCOL_H
#define LIBSIGROK_HARDWARE_PSLAB_PROTOCOL_H

#include <stdint.h>
#include <glib.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"

#define LOG_PREFIX "pslab"
#define NUM_ANALOG_CHANNELS 8

#define BUFSIZE 10000
#define MAX_SAMPLES 10000
#define COMMON 0x0b
#define VERSION_COMMAND 0x05

#define ADC 0x02
#define CAPTURE_ONE 0x01
#define CAPTURE_TWO 0x02
#define CAPTURE_DMASPEED 0x03
#define CAPTURE_FOUR 0x04
#define CONFIGURE_TRIGGER 0x05
#define GET_CAPTURE_STATUS 0x06
#define GET_CAPTURE_CHANNEL 0x07
#define SET_PGA_GAIN 0x08
#define GET_VOLTAGE 0x09
#define GET_VOLTAGE_SUMMED 0x0a
#define START_ADC_STREAMING 0x0b
#define SELECT_PGA_CHANNEL 0x0c
#define CAPTURE_12BIT 0x0d
#define CAPTURE_MULTIPLE 0x0e
#define SET_HI_CAPTURE 0x0f
#define SET_LO_CAPTURE 0x10

#define RETRIEVE_BUFFER 0x08

struct dev_context {
	/* device mode */
	int mode;

	/* trigger */
	gboolean trigger_enabled;
	const struct sr_channel *trigger_channel;
	double trigger_voltage;

	/* Acquisition settings */
	uint64_t samplerate; // Time gap between samples in microseconds
	gboolean data_source;
	GSList * enabled_channels;
	const struct sr_channel *channel_one_map;
	gboolean ch_enabled[NUM_ANALOG_CHANNELS];
	struct sr_sw_limits limits;
	unsigned char buf[BUFSIZE];
	int buflen;

	/* GSList entry for the current channel. */
	GSList *channel_entry;

	/* Acq buffers used for reading from the scope and sending data to app */
	uint16_t *short_int_buffer;
	double *data;
};

struct analog_channel {
	const char *name;

	int index;

	int chosa;

	double minInput;

	double maxInput;
};

struct channel_priv {
	uint64_t samples_in_buffer;
	int buffer_idx;
	int chosa;
	double min_input;
	double max_input;
	uint64_t gain;
	int programmable_gain_amplifier;
	double resolution;
};

SR_PRIV int pslab_receive_data(int fd, int revents, void *cb_data);
SR_PRIV char* pslab_get_version(struct sr_serial_dev_inst* serial, uint8_t c1, uint8_t c2);

SR_PRIV int pslab_update_coupling(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_update_samplerate(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_update_vdiv(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_update_channels(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_init(const struct sr_dev_inst *sdi);
SR_PRIV int set_gain(const struct sr_dev_inst *sdi, const struct sr_channel *ch, uint64_t gain);
SR_PRIV void set_resolution(const struct sr_channel *ch, int resolution);
SR_PRIV int get_ack(const struct sr_dev_inst *sdi);
SR_PRIV void configure_trigger(const struct sr_dev_inst *sdi);
SR_PRIV void caputure_oscilloscope(const struct sr_dev_inst *sdi);
SR_PRIV int fetch_data(const struct sr_dev_inst *sdi);
SR_PRIV gboolean progress(const struct sr_dev_inst *sdi);
SR_PRIV double scale(const struct sr_channel *ch, int raw_value);
SR_PRIV int unscale(const struct sr_channel *ch, double voltage);
SR_PRIV struct dev_context *pslab_dev_new();

#endif
