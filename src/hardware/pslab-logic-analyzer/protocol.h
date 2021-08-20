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

#define NUM_DIGITAL_INPUT_CHANNEL 8
#define MIN_EVENTS 10
#define MAX_EVENTS 2500
#define COMMON 0x0b
#define ADC 0x02
#define VERSION_COMMAND 0x05

static const uint64_t PRESCALERS[] = {1, 8, 64, 256};

enum trigger_pattern {
    PSLAB_TRIGGER_PATTERN_DISABLED,
    PSLAB_TRIGGER_PATTERN_FALLING,
    PSLAB_TRIGGER_PATTERN_RISING,
    PSLAB_TRIGGER_PATTERN_ANY,
    PSLAB_TRIGGER_PATTERN_FOUR_RISING,
    PSLAB_TRIGGER_PATTERN_SIXTEEN_RISING,
};

struct dev_context {
    /* trigger */
    gboolean trigger_enabled;
    struct sr_channel trigger_channel;
    char *trigger_pattern;

    uint64_t e2e_time;
    GSList * enabled_channels;
    struct sr_channel *channel_one_map;
    struct sr_channel *channel_two_map;
    struct sr_sw_limits limits;
    int prescaler;
    int trimmed;
    int trigger_pattern_number;

    /* GSList entry for the current channel. */
    GSList *channel_entry;

    /* Acq buffers used for reading from the scope and sending data to app */
    uint16_t *short_int_buffer;
    float *data;
};

struct digital_input_channel {
    const char *name;
};

struct channel_priv {
    char *datatype;
    int events_in_buffer;
    int buffer_idx;
};

struct channel_group_priv {
    const char *logic_mode;
};

SR_PRIV int pslab_logic_analyzer_receive_data(int fd, int revents, void *cb_data);
SR_PRIV char* pslab_get_version(struct sr_serial_dev_inst* serial);
SR_PRIV void pslab_write_u8(struct sr_serial_dev_inst* serial, uint8_t cmd[], int count);
SR_PRIV void pslab_write_u16(struct sr_serial_dev_inst* serial, uint16_t val[], int count);
SR_PRIV int assign_channel(const char* channel_name,
			   struct sr_channel *target, GSList* list);

#endif
