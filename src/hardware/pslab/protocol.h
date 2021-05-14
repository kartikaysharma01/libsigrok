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

#define NUM_CHANNELS		8
// #define NUM_TRIGGER_STAGES	16

enum pslab_edge_modes {
    DS_EDGE_RISING,
    DS_EDGE_FALLING,
};

struct pslab_profile {
    uint16_t vid;
    uint16_t pid;

    const char *vendor;
    const char *model;
    const char *model_version;

};

struct dev_context {
    const struct dslogic_profile *profile;

    const uint64_t *samplerates;

    uint64_t current_samplerate;
    uint64_t limit_samples;

    unsigned int sent_samples;
    int submitted_transfers;
    int empty_transfer_count;

//    unsigned int num_transfers;
//    struct libusb_transfer **transfers;
    struct sr_context *ctx;

    int clock_edge;
    double current_threshold;
};

SR_PRIV int pslab_dev_open(struct sr_dev_inst *sdi, struct sr_dev_driver *di);
SR_PRIV struct dev_context *dslogic_dev_new(void);
SR_PRIV int pslab_acquisition_start(const struct sr_dev_inst *sdi);
SR_PRIV int pslab_acquisition_stop(struct sr_dev_inst *sdi);

#endif
