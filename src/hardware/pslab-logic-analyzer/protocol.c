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

SR_PRIV int pslab_logic_analyzer_receive_data(int fd, int revents, void *cb_data)
{
	const struct sr_dev_inst *sdi;
	struct dev_context *devc;

	(void)fd;

	if (!(sdi = cb_data))
		return TRUE;

	if (!(devc = sdi->priv))
		return TRUE;

	if (revents == G_IO_IN) {
		/* TODO */
	}

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
