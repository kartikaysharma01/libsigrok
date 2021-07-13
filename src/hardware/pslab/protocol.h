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
#define NUM_ANALOG_CHANNELS 4
#define NUM_DIGITAL_OUTPUT_CHANNEL 4
#define HIGH_FREQUENCY_LIMIT 1e7
#define CLOCK_RATE 64e6


#define MAX_SAMPLES 10000
#define COMMON 0x0b
#define MIN_SAMPLES 10

#define ADC 0x02
#define VERSION_COMMAND 0x05
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

/*--------WAVEGEN-----*/
#define WAVEGEN 0x07
#define SET_WG 0x01
#define SET_SQR1 0x03
#define SET_SQR2 = 0x04
#define SET_SQRS 0x05
#define TUNE_SINE_OSCILLATOR 0x06
#define SQR4 0x07
#define MAP_REFERENCE 0x08
#define SET_BOTH_WG 0x09
#define SET_WAVEFORM_TYPE 0x0a
#define SELECT_FREQ_REGISTER 0x0b
#define DELAY_GENERATOR 0x0c
#define SET_SINE1 0x0d
#define SET_SINE2 0x0e

/*-----digital outputs----*/
#define DOUT 0x08
#define SET_STATE 0x01


static const uint8_t GAIN_VALUES[] = {1, 2, 4, 5, 8, 10, 16, 32};

static const uint64_t PRESCALERS[] = {1, 8, 64, 256};

struct dev_context {
	/* device mode */
	int mode;

	/* trigger */
	gboolean trigger_enabled;
	struct sr_channel *trigger_channel;
	double trigger_voltage;

	/* PWM generator */
	double frequency;
	GSList * enabled_digital_output;
	gboolean pwm;
	int wavelength;
	int prescaler;

	/* Acquisition settings */
	uint64_t samplerate;
	GSList * enabled_channels_analog;
	struct sr_channel *channel_one_map;
	struct sr_sw_limits limits;

	/* GSList entry for the current channel. */
	GSList *channel_entry;

	/* Acq buffers used for reading from the scope and sending data to app */
	uint16_t *short_int_buffer;
	float *data;
};

struct analog_channel {
	const char *name;
	int index;
	int chosa;
	double minInput;
	double maxInput;
};

struct channel_priv {
	int buffer_idx;
	int chosa;
	double min_input;
	double max_input;
	uint16_t gain;
	int programmable_gain_amplifier;
	double resolution;
};

struct channel_group_priv {
	int range;
};


struct digital_output_channel {
    const char *name;
    uint8_t state_mask;
};

struct digital_output_cg_priv {
    double duty_cycle;
    double phase;
    char *state;
    uint8_t state_mask;
};

SR_PRIV int pslab_receive_data(int fd, int revents, void *cb_data);
SR_PRIV char* pslab_get_version(struct sr_serial_dev_inst* serial);
SR_PRIV int pslab_set_gain(const struct sr_dev_inst *sdi,
	const struct sr_channel *ch, uint16_t gain);
SR_PRIV void pslab_set_resolution(const struct sr_channel *ch, int resolution);
SR_PRIV int pslab_get_ack(const struct sr_dev_inst *sdi);
SR_PRIV void pslab_configure_trigger(const struct sr_dev_inst *sdi);
SR_PRIV void pslab_caputure_oscilloscope(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_generate_pwm(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_get_wavelength(double frequency, int table_size, int *timegap, int *prescaler);
SR_PRIV int pslab_set_state(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_fetch_data(const struct sr_dev_inst *sdi);
SR_PRIV gboolean pslab_progress(const struct sr_dev_inst *sdi);
SR_PRIV float pslab_scale(const struct sr_channel *ch, uint16_t raw_value);
SR_PRIV int assign_channel(const char* channel_name,
			   struct sr_channel *target, GSList* list);
SR_PRIV int pslab_unscale(const struct sr_channel *ch, double voltage);
SR_PRIV void pslab_write_u8(struct sr_serial_dev_inst* serial, uint8_t buf[], int count);
SR_PRIV void pslab_write_u16(struct sr_serial_dev_inst* serial, uint16_t val[], int count);

#endif
