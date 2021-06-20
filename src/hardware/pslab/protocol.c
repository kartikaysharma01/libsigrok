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

SR_PRIV int pslab_receive_data(int fd, int revents, void *cb_data)
{
	const struct sr_dev_inst *sdi;
	struct dev_context *devc;
	struct sr_serial_dev_inst *serial;
	gboolean stop = FALSE;
	int len;

	(void)fd;

	if (!(sdi = cb_data))
		return TRUE;

	if (!(devc = sdi->priv))
		return TRUE;

	serial = sdi->conn;
	if (revents == G_IO_IN) {
		/* Serial data arrived. */
		while (BUFSIZE - devc->buflen - 1 > 0) {
			len = serial_read_nonblocking(serial, devc->buf + devc->buflen, 1);
			if (len < 1)
				break;
			devc->buflen += len;
			*(devc->buf + devc->buflen) = '\0';
			if (*(devc->buf + devc->buflen - 1) == '\n') {
				/* End of line */
//				stop = receive_line(sdi);
				break;
			}
		}
	}

//	if (sr_sw_limits_check(&devc->limits) || stop)
//		sr_dev_acquisition_stop(sdi);
//	else
//		dispatch(sdi);


	return TRUE;
}

SR_PRIV char* pslab_get_version(struct sr_serial_dev_inst* serial, uint8_t c1, uint8_t c2 )
{
	char *buffer = g_malloc0(16);
	int len = 15;
	serial_write_blocking(serial,&c1, sizeof(c1), serial_timeout(serial, sizeof(c1)));
	serial_write_blocking(serial,&c2, sizeof(c2),serial_timeout(serial, sizeof(c2)));
	serial_readline(serial, &buffer, &len, serial_timeout(serial, sizeof(buffer)));
	return buffer;
}

SR_PRIV int pslab_update_coupling(const struct sr_dev_inst *sdi)
{
	return SR_OK;
}

SR_PRIV int pslab_update_samplerate(const struct sr_dev_inst *sdi)
{
//	struct dev_context *devc = sdi->priv;
//	sr_dbg("update samplerate %d", samplerate_to_reg(devc->samplerate));
//
//	return write_control(sdi, SAMPLERATE_REG, samplerate_to_reg(devc->samplerate));

	return SR_OK;
}

SR_PRIV int pslab_update_vdiv(const struct sr_dev_inst *sdi)
{
	return SR_OK;
}

SR_PRIV int pslab_update_channels(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc = sdi->priv;
	struct sr_serial_dev_inst *serial = sdi->conn;
	devc -> enabled_channels;
	return SR_OK;
}

SR_PRIV int pslab_init(const struct sr_dev_inst *sdi)
{
	sr_dbg("Initializing");

	pslab_update_samplerate(sdi);
	pslab_update_vdiv(sdi);
	pslab_update_coupling(sdi);
	pslab_update_channels(sdi);
	// hantek_6xxx_update_channels(sdi); /* Only 2 channel mode supported. */

	return SR_OK;
}