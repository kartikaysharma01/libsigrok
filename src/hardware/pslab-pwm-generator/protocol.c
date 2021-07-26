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
#include <math.h>
#include "protocol.h"

SR_PRIV int pslab_pwm_generator_receive_data(int fd, int revents, void *cb_data)
{
	struct sr_dev_inst *sdi;
	struct dev_context *devc;
	int ret;

	(void)fd;
	(void)revents;

	if (!(sdi = cb_data))
		return TRUE;

	if (!(devc = sdi->priv))
		return TRUE;

	ret = pslab_generate_pwm(sdi);
	if (ret != SR_OK) {
		sr_info("Error generating wave: %d", ret);
		sr_dev_acquisition_stop(sdi);
		return TRUE;
	}

	ret = pslab_set_state(sdi);
	if (ret != SR_OK) {
		sr_info("Error setting state: %d", ret);
		sr_dev_acquisition_stop(sdi);
		return TRUE;
	}

	devc->enabled_digital_output = NULL;
	g_slist_free(devc->enabled_digital_output);
	std_session_send_df_frame_end(sdi);
	sr_dev_acquisition_stop(sdi);

	return TRUE;
}

SR_PRIV void pslab_write_u8(struct sr_serial_dev_inst* serial, uint8_t cmd[], int count)
{
	int i, bytes_written;
	for (i = 0; i < count; i++) {
		bytes_written = serial_write_blocking(serial, &cmd[i], 1,
						      serial_timeout(serial, 1));

		if (bytes_written < 1)
			sr_dbg("Failed to write command %d to device.", cmd[i]);
	}

}

SR_PRIV void pslab_write_u16(struct sr_serial_dev_inst* serial, uint16_t val[], int count)
{
	int i, bytes_written;
	for (i = 0; i < count; i++) {
		bytes_written = serial_write_blocking(serial, &val[i], 2,
						      serial_timeout(serial, 2));
		if (bytes_written < 2)
			sr_dbg("Failed to write command %d to device.", val[i]);
	}
}

SR_PRIV char* pslab_get_version(struct sr_serial_dev_inst* serial)
{
	sr_info("Sending version commands to device");

	uint8_t cmd[] = {COMMON, VERSION_COMMAND};
	char *buffer = g_malloc0(16);
	int len = 15;
	pslab_write_u8(serial, cmd, 2);
	serial_readline(serial, &buffer, &len, serial_timeout(serial, sizeof(buffer)));
	return buffer;
}

SR_PRIV int pslab_get_ack(const struct sr_dev_inst *sdi)
{
	struct sr_serial_dev_inst *serial;
	int *buf;

	serial = sdi->conn;
	buf = g_malloc0(1);
	serial_read_blocking(serial,buf,1, serial_timeout(serial,1));

	if (!(*buf & 0x01) || !(*buf)) {
		sr_dbg("Did not receive ACK or Received non ACK byte while waiting for ACK.");
		g_free(buf);
		return SR_ERR_IO;
	}

	g_free(buf);
	return SR_OK;
}

SR_PRIV int pslab_generate_pwm(const struct sr_dev_inst *sdi)
{
	sr_info("Generate PWM Wave");

	struct dev_context *devc;
	struct sr_channel_group *cg;
	struct sr_serial_dev_inst *serial;
	struct channel_group_priv *cp;
	GSList *l;
	uint16_t phases[4], duty[4];
	int i, prescaler_idx;

	devc = sdi->priv;
	serial = sdi->conn;

	if (pslab_get_wavelength(devc->frequency, 1, &devc->wavelength, &devc->prescaler) != SR_OK)
		return SR_ERR_ARG;

	sr_dbg("wavelength == %d , prescaler  == %d , frequecvy == %f ", devc->wavelength, devc->prescaler, devc->frequency);

	for(l = sdi->channel_groups, i = 0; l; l = l->next, i++) {
		cg = l->data;
		cp = cg->priv;

		sr_dbg("channel grp  == %s , duty cycle  == %f , phase == %f , devc->wavelength = %d", cg->name,
		       cp->duty_cycle, cp->phase, devc->wavelength);

		duty[i] = (int)(fmod(cp->duty_cycle + cp->phase, 1) * devc->wavelength);
		sr_dbg("ln 144 : calculated duty_cycle == %d", duty[i]);
		duty[i] = MAX(1, duty[i] - 1);
		sr_dbg("ln 144 : calculated duty_cycle after MAX == %d", duty[i]);
		phases[i] = (int)(fmod(cp->phase, 1) * devc->wavelength);
		phases[i] = MAX(0, phases[i] - 1);
	}

	for(int j =0 ; j<4 ; j++) {
		if (j ==0) {
			sr_dbg("wavelength - 1 == %d", devc->wavelength - 1);
			sr_dbg("duty_cycles[%d] == %d", j, duty[j]);
			continue;
		}

		sr_dbg("phases[%d] == %d", j , phases[j]);
		sr_dbg("duty_cycles[%d] == %d", j , duty[j]);
	}
	// remove this
	prescaler_idx = std_u64_idx(g_variant_new_uint64(devc->prescaler), PRESCALERS, 4);
	sr_dbg("_PRESCALERS.index(prescaler) | continuous == %d ", prescaler_idx | (1 << 5));

	uint8_t cmd[] = {WAVEGEN, SQR4};
	pslab_write_u8(serial, cmd, 2);

	uint16_t val[] = {devc->wavelength - 1, duty[0], phases[1], duty[1], phases[2], duty[2], phases[3], duty[3]};
	pslab_write_u16(serial, val, 8);

	prescaler_idx = std_u64_idx(g_variant_new_uint64(devc->prescaler), PRESCALERS, 4);

	if (prescaler_idx < 0) {
		sr_dbg("Invalid prescaler value");
		return SR_ERR_ARG;
	}

	uint8_t cmd1[] = {prescaler_idx | (1 << 5)};
	pslab_write_u8(serial, cmd1, 1);

	if (pslab_get_ack(sdi) != SR_OK) {
		sr_err("Did not receive ACK byte for PWM generate commands");
		return SR_ERR;
	}

	return SR_OK;
}

SR_PRIV int pslab_get_wavelength(double frequency, int table_size, int *wavelength, int *prescaler)
{
	sr_info("Calculate wavelength for PWM gen, frequency = %f table_size = %d", frequency, table_size);
	int i, wl;

	for (i = 0; i < NUM_DIGITAL_OUTPUT_CHANNEL; i++) {
		wl = (int) round(CLOCK_RATE / frequency / (double)PRESCALERS[i] / table_size);

		if(wl > 0 && wl < (int)pow(2,16)) {
			*wavelength = wl;
			*prescaler = (int)PRESCALERS[i];
			return SR_OK;
		}
	}

	sr_err("Prescaler out of range, could not calculate wavelength");
	return SR_ERR_ARG;
}

SR_PRIV int pslab_set_state(const struct sr_dev_inst *sdi)
{
	struct sr_channel_group *cg;
	struct sr_channel *ch;
	struct sr_serial_dev_inst *serial;
	struct channel_group_priv *cp;
	GSList *l;
	int states, sq;

	states = 0;
	serial = sdi->conn;

	for(l = sdi->channel_groups; l; l = l->next) {
		cg = l->data;
		ch = cg->channels->data;
		cp = cg->priv;
		if(g_strcmp0(cp->state, "LOW"))
			sq = 0;
		else if(g_strcmp0(cp->state, "HIGH"))
			sq = 1;

		cp->duty_cycle = sq;
		states |= cp->state_mask | (sq << ch->index);
	}

	sr_dbg("inside set_state, states = %d", states);
	uint8_t cmd[] = {DOUT, SET_STATE, states};
	pslab_write_u8(serial, cmd, 3);

	if (pslab_get_ack(sdi) != SR_OK) {
		sr_err("Did not receive ACK byte for set state");
		return SR_ERR;
	}

	return SR_OK;
}

