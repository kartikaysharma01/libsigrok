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
	int len = 10;

	(void)fd;

	if (!(sdi = cb_data))
		return TRUE;


	serial = sdi->conn;
	if (revents == G_IO_IN) {
		/* Serial data arrived. */
		short int * buf = g_malloc0(2);
		serial_read_nonblocking(serial,buf,2);
		printf("outpiut %d \n",*buf);
		printf("line 42\n");

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

	uint8_t *commands;
	commands = g_malloc0(sizeof(uint8_t));
	*commands = ADC;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = CAPTURE_DMASPEED;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = 3 & 0xff;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	short int samplecount = 1000;
	short int timegap = 4;
	serial_write_blocking(serial,&samplecount, sizeof (samplecount), serial_timeout(serial, sizeof (samplecount)));
	serial_write_blocking(serial,&timegap, sizeof (timegap), serial_timeout(serial, sizeof (timegap)));

	char *buf = g_malloc0(1);
	serial_read_blocking(serial,buf,1,100);
	printf("ack byte %s\n",buf);
	uint8_t x = *buf;
	printf("test %d\n", x & 0x01);

	g_usleep(4000);

	*commands = COMMON;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = RETRIEVE_BUFFER;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	short int startingposition = 0;
	serial_write_blocking(serial,&startingposition, sizeof (startingposition), serial_timeout(serial, sizeof (startingposition)));
	serial_write_blocking(serial,&samplecount, sizeof (samplecount), serial_timeout(serial, sizeof (samplecount)));
	return SR_OK;
}

SR_PRIV int pslab_init(const struct sr_dev_inst *sdi)
{
	sr_dbg("Initializing");

	pslab_update_samplerate(sdi);
	pslab_update_vdiv(sdi);
	pslab_update_coupling(sdi);
//	pslab_update_channels(sdi);
	// hantek_6xxx_update_channels(sdi); /* Only 2 channel mode supported. */

	return SR_OK;
}