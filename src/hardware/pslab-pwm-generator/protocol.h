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

#ifndef LIBSIGROK_HARDWARE_PSLAB_PWM_GENERATOR_PROTOCOL_H
#define LIBSIGROK_HARDWARE_PSLAB_PWM_GENERATOR_PROTOCOL_H

#include <stdint.h>
#include <glib.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"

#define LOG_PREFIX "pslab-pwm-generator"

#define NUM_DIGITAL_OUTPUT_CHANNEL 4
#define HIGH_FREQUENCY_LIMIT 10e7
#define CLOCK_RATE 64e6
#define COMMON 0x0b
#define ADC 0x02
#define VERSION_COMMAND 0x05

/*--------WAVEGEN-----*/
#define WAVEGEN 0x07
#define SET_WG 0x01
#define SET_SQR1 0x03
#define SET_SQR2 0x04
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

static const uint64_t PRESCALERS[] = {1, 8, 64, 256};

struct dev_context {
    /* PWM generator */
    double frequency;
    GSList * enabled_digital_output;
    gboolean pwm;
    int wavelength;
    int prescaler;
};

struct digital_output_channel {
    const char *name;
    uint8_t state_mask;
};

struct channel_group_priv {
    double duty_cycle;
    double phase;
    char *state;
    uint8_t state_mask;
};

SR_PRIV int pslab_pwm_generator_receive_data(int fd, int revents, void *cb_data);
SR_PRIV void pslab_write_u8(struct sr_serial_dev_inst* serial, uint8_t cmd[], int count);
SR_PRIV void pslab_write_u16(struct sr_serial_dev_inst* serial, uint16_t val[], int count);
SR_PRIV char* pslab_get_version(struct sr_serial_dev_inst* serial);
SR_PRIV int pslab_get_ack(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_generate_pwm(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_get_wavelength(double frequency, int table_size, int *wavelength, int *prescaler);
SR_PRIV int pslab_set_state(const struct sr_dev_inst *sdi);

#endif
