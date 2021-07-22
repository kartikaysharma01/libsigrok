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

#ifndef LIBSIGROK_HARDWARE_PSLAB_LOGIC_ANALYZER_PROTOCOL_H
#define LIBSIGROK_HARDWARE_PSLAB_LOGIC_ANALYZER_PROTOCOL_H

#include <stdint.h>
#include <glib.h>
#include <libsigrok/libsigrok.h>
#include "libsigrok-internal.h"

#define LOG_PREFIX "pslab-logic-analyzer"

#define NUM_DIGITAL_OUTPUT_CHANNEL 4
#define COMMON 0x0b
#define ADC 0x02
#define VERSION_COMMAND 0x05

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

SR_PRIV int pslab_logic_analyzer_receive_data(int fd, int revents, void *cb_data);

#endif
