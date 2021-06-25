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

static const uint64_t GAIN_VALUES[] = {1, 2, 4, 5, 8, 10, 16, 32};

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

SR_PRIV void caputure_oscilloscope(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc = sdi->priv;
	struct sr_serial_dev_inst *serial = sdi->conn;
	int i;
	GSList *l;

	struct channel_priv *cp = devc->channel_one_map.priv;
	cp->resolution =10;
	int chosa = cp->chosa;
	cp->samples_in_buffer = devc->limits.limit_samples;
	cp->buffer_idx = 0;

	uint8_t *commands;
	commands = g_malloc0(sizeof(uint8_t));
	*commands = ADC;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));

	if(g_slist_length(devc->enabled_channels) == 1)
	{
		if(devc->trigger_enabled)
		{
			*commands = CAPTURE_ONE;
			serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
			*commands = chosa | 0x80;
			serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));

		}
		if(devc->samplerate <= 1000)
		{
			cp->resolution = 12;
			*commands = CAPTURE_DMASPEED;
			serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
			*commands = chosa | 0x80;
			serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
		}
		else
		{
			*commands = CAPTURE_DMASPEED;
			serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
			*commands = chosa;
			serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
		}
	}
	else if(g_slist_length(devc->enabled_channels) == 2)
	{
		for(l=devc->enabled_channels; l; l=l->next)
		{
			struct sr_channel *ch = l->data;
			if(!g_strcmp0(ch->name, "CH2"))
			{
				struct channel_priv *cp2 = ch->priv;
				cp2->resolution = 10;
				cp2->samples_in_buffer = devc->limits.limit_samples;
				cp2->buffer_idx = 1 * devc->limits.limit_samples;
				break;
			}
		}
		*commands = CAPTURE_TWO;
		serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
		*commands = chosa | (0x80 * devc->trigger_enabled);
		serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	}
	else
	{
		for(i=0, l=devc->enabled_channels; l; l=l->next,i++)
		{
			struct sr_channel *ch = l->data;
			if(g_strcmp0(ch->name, "CH1"))
			{
				struct channel_priv *cp2 = ch->priv;
				cp2->resolution = 10;
				cp2->samples_in_buffer = devc->limits.limit_samples;
				cp2->buffer_idx = (i+1) * devc->limits.limit_samples;
			}
		}
		*commands = CAPTURE_FOUR;
		serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
		*commands = chosa | (0 << 4) | (0x80 * devc->trigger_enabled);
		serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	}

	short int samplecount = devc->limits.limit_samples;
	short int samplerate = devc->samplerate;
	serial_write_blocking(serial,&samplecount, sizeof (samplecount), serial_timeout(serial, sizeof (samplecount)));
	serial_write_blocking(serial,&samplerate, sizeof (samplerate), serial_timeout(serial, sizeof (samplerate)));
	get_ack(sdi);

	// test
	g_usleep(devc->limits.limit_samples / devc->samplerate);

	*commands = COMMON;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = RETRIEVE_BUFFER;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	short int startingposition = 0;
	serial_write_blocking(serial,&startingposition, sizeof (startingposition), serial_timeout(serial, sizeof (startingposition)));
	serial_write_blocking(serial,&samplecount, sizeof (samplecount), serial_timeout(serial, sizeof (samplecount)));
//	return SR_OK;


//	fetch_data(sdi);

}

SR_PRIV void fetch_data(const struct sr_dev_inst *sdi)
{

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

SR_PRIV uint64_t lookup_maximum_samplerate(guint channels)
{
	static const uint64_t channels_idx[][2] = {
			{1, 0},
			{2, 1},
			{4, 2},
	};
	static const uint64_t min_samplerates[][2] = {
			{2000, 1333},
			{1142, 1142},
			{571, 571},
	};
	/* TODO: revisit the formulae, not correct rn */
	return min_samplerates[channels_idx[channels-1][1]][0];
}

SR_PRIV int check_args(guint channels,uint64_t samples ,uint64_t samplerate)
{
	if(channels > 4)
	{
		sr_dbg("Number of channels to sample must be 1, 2, 3, or 4");
		return SR_ERR_IO;
	}

	if(samples<0 || samples > (MAX_SAMPLES/channels))
	{
		sr_dbg("Invalid number of samples");
		return SR_ERR_IO;
	}

	if(samplerate > lookup_maximum_samplerate(channels))
	{
		sr_dbg("Samplerate must be less than %lu", lookup_maximum_samplerate(channels));
		return SR_ERR_IO;
	}

	return SR_OK;
}

SR_PRIV void set_gain(const struct sr_dev_inst *sdi, const struct sr_channel *ch, uint64_t gain)
{
	struct sr_serial_dev_inst *serial = sdi->conn;
	struct channel_priv *cp = ch->priv;
	cp->gain = gain;
	int pga = cp->programmable_gain_amplifier;
	int gain_idx = std_u64_idx((GVariant *) gain, GAIN_VALUES, 8);
	uint8_t *commands = g_malloc0(sizeof(uint8_t));
	*commands = ADC;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = SET_PGA_GAIN;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = pga;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = gain_idx;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));

	get_ack(sdi);
}

SR_PRIV void get_ack(const struct sr_dev_inst *sdi)
{
	struct sr_serial_dev_inst *serial = sdi->conn;
	int *buf = g_malloc0(1);
	serial_read_blocking(serial,buf,1, serial_timeout(serial,1));

	if(!(*buf & 0x01))
		sr_dbg("Received non ACK byte while waiting for ACK.");
}