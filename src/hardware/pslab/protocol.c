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
#include <math.h>
#include "protocol.h"

static const uint8_t GAIN_VALUES[] = {1, 2, 4, 5, 8, 10, 16, 32};

SR_PRIV int pslab_receive_data(int fd, int revents, void *cb_data)
{
	struct sr_dev_inst *sdi;
	struct sr_serial_dev_inst *serial;
	struct dev_context *devc;
	struct sr_datafeed_packet packet;
	struct sr_datafeed_analog analog;
	struct sr_analog_encoding encoding;
	struct sr_analog_meaning meaning;
	struct sr_analog_spec spec;
	struct sr_datafeed_logic logic;
	int len, i, vref;
	struct sr_channel *ch;
	gsize expected_data_bytes;

	(void)fd;

	if (!(sdi = cb_data))
		return TRUE;

	if (!(devc = sdi->priv))
		return TRUE;

	serial = sdi->conn;

	if (!(revents == G_IO_IN || revents == 0))
		return TRUE;

	ch = devc->channel_entry->data;

	if(fetch_data(sdi) != SR_OK)
		return TRUE;

	devc->short_int_buffer = g_malloc(2);
	devc->data = g_malloc0((int)devc->limits.limit_samples * sizeof(float));
	int samples_collected =0;

	for (i = 0; i < (int)devc->limits.limit_samples; i++) {
		if(serial_read_blocking(serial, devc->short_int_buffer, 2 , serial_timeout(serial, 2)) < 2)
			break;
		sr_dbg("ln 59, raw value == %d , and voltage == %f ", *devc->short_int_buffer, scale(ch, *devc->short_int_buffer));
		devc->data[i] = scale(ch, *devc->short_int_buffer);
		samples_collected = i;
	}
	get_ack(sdi);

	sr_analog_init(&analog, &encoding, &meaning, &spec, 6);
	analog.meaning->channels = g_slist_append(NULL, ch);
	analog.num_samples = samples_collected + 1;
	analog.data = devc->data;
	analog.meaning->mq = SR_MQ_VOLTAGE;
	analog.meaning->unit = SR_UNIT_VOLT;
	analog.meaning->mqflags = 0;
	analog.encoding->unitsize = sizeof(float);
	analog.encoding->is_float = TRUE;
	analog.encoding->is_signed = TRUE;

	packet.type = SR_DF_ANALOG;
	packet.payload = &analog;
	sr_session_send(sdi, &packet);
	g_slist_free(analog.meaning->channels);

	if (devc->channel_entry->next) {
		/* We got the frame for this channel, now get the next channel. */
		devc->channel_entry = devc->channel_entry->next;
//		fetch_data(sdi);
	} else {
		/* Done with this frame. */
		std_session_send_df_frame_end(sdi);
		sr_dev_acquisition_stop(sdi);

	}
//	if (revents == G_IO_IN) {
//		/* Serial data arrived. */
//		uint16_t *buf = g_malloc0(2);
//		serial_read_blocking(serial,buf,2, serial_timeout(serial,2));
//		uint16_t output = *buf;
//		sr_dbg("ln 198 output == %d", output);
//	}

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
	// invalidate_buffer()
	struct dev_context *devc = sdi->priv;
	struct sr_serial_dev_inst *serial = sdi->conn;
	int i;
	GSList *l;

	struct channel_priv *cp_map = devc->channel_one_map->priv;
	set_resolution(devc->channel_one_map,10);
	int chosa = cp_map->chosa;
	cp_map->samples_in_buffer = devc->limits.limit_samples;
	cp_map->buffer_idx = 0;

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
		if(devc->samplerate <= 1000000)
		{
			set_resolution(devc->channel_one_map,12);
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
	else if(g_slist_length(devc->enabled_channels) == 2) {
		for(l=devc->enabled_channels; l; l=l->next) {
			struct sr_channel *ch = l->data;
			if(!g_strcmp0(ch->name, "CH2")) {
				struct channel_priv *cp = ch->priv;
				set_resolution(ch,10);
				cp->samples_in_buffer = devc->limits.limit_samples;
				cp->buffer_idx = (int)devc->limits.limit_samples;
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
				struct channel_priv *cp = ch->priv;
				set_resolution(ch,10);
				cp->samples_in_buffer = devc->limits.limit_samples;
				cp->buffer_idx = (i) * (int)devc->limits.limit_samples;
			}
		}
		*commands = CAPTURE_FOUR;
		serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
		*commands = chosa | (0 << 4) | (0x80 * devc->trigger_enabled);
		serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	}

	uint16_t samplecount = devc->limits.limit_samples;
	uint16_t timegap = (int)(8000000/devc->samplerate);
	serial_write_blocking(serial,&samplecount, 2, serial_timeout(serial, 2));
	serial_write_blocking(serial,&timegap, 2, serial_timeout(serial, 2));

	if (get_ack(sdi) != SR_OK)
		sr_dbg("Failed to capture samples");

	// test
	g_usleep(1000000 * devc->limits.limit_samples / devc->samplerate);

	while(!progress(sdi))
		continue;

//	*commands = COMMON;
//	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
//	*commands = RETRIEVE_BUFFER;
//	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
//	uint16_t startingposition = 0 ;
//	serial_write_blocking(serial,&startingposition, 2, serial_timeout(serial, 2));
//	uint16_t samples = samplecount ;
//	serial_write_blocking(serial,&samplecount, 2, serial_timeout(serial,  2));

//	for(int j=0; j<samplecount; j++)
//	{
//		uint16_t *buf2 = g_malloc0(2);
//		serial_read_blocking(serial,buf2,2, serial_timeout(serial,2));
//		uint16_t output = *buf2;
//		sr_dbg("ln 198 output == %d", output);
//	}
//
//	if (get_ack(sdi) == SR_OK)
//		sr_dbg("Successful to fetch buffer");


//	return SR_OK;


//	fetch_data(sdi);

}

SR_PRIV int fetch_data(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc;
	struct sr_channel *ch;
	struct sr_serial_dev_inst *serial;

	if (!(devc = sdi->priv))
		return SR_ERR;

	ch = devc->channel_entry->data;
	serial = sdi->conn;

	sr_dbg("Starting reading data from channel %s", ch->name);

	uint8_t *commands;
	commands = g_malloc0(sizeof(uint8_t));
	*commands = COMMON;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = RETRIEVE_BUFFER;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	uint16_t startingposition =  ((struct channel_priv *)(ch->priv))->buffer_idx;
	sr_dbg("ln 287 %hu", startingposition);
	serial_write_blocking(serial,&startingposition, 2, serial_timeout(serial, 2));
	uint16_t samples = devc->limits.limit_samples ;
	serial_write_blocking(serial,&samples, 2, serial_timeout(serial,  2));

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


SR_PRIV void set_resolution(const struct sr_channel *ch, int resolution)
{
	struct channel_priv *cp = ch->priv;
	cp->resolution = pow(2,resolution) - 1;
}

SR_PRIV gboolean progress(const struct sr_dev_inst *sdi)
{
	struct sr_serial_dev_inst *serial = sdi->conn;
	uint8_t *commands = g_malloc0(sizeof(uint8_t));
	*commands = ADC;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = GET_CAPTURE_STATUS;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));

	uint8_t *buf = g_malloc0(1);
	serial_read_blocking(serial,buf,1, serial_timeout(serial,1));
	uint8_t capturing_complete = *buf;
	g_free(buf);
	uint16_t *buf2 = g_malloc0(2);
	serial_read_blocking(serial,buf2,2, serial_timeout(serial,2));
	uint16_t samples_read = *buf2;

	if (get_ack(sdi) != SR_OK)
		sr_dbg("Failed in knowing capturing status");

	return capturing_complete;
}

SR_PRIV int set_gain(const struct sr_dev_inst *sdi, const struct sr_channel *ch, uint64_t gain)
{
	if(!(!g_strcmp0(ch->name,"CH1") || !g_strcmp0(ch->name,"CH2"))) {
		sr_dbg("Analog gain is not available on %s", ch->name);
		return SR_ERR_ARG;
	}

	struct sr_serial_dev_inst *serial = sdi->conn;
	struct channel_priv *cp = ch->priv;
	cp->gain = gain;
	uint8_t pga = cp->programmable_gain_amplifier;
	uint8_t gain_idx = std_u8_idx(g_variant_new_uint64(gain), GAIN_VALUES, 8);

	if(gain_idx < 0) {
		sr_dbg("Invalid gain value");
		return SR_ERR_ARG;
	}

	uint8_t *commands = g_malloc0(sizeof(uint8_t));
	*commands = ADC;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = SET_PGA_GAIN;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = pga;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = gain_idx;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));

	if(get_ack(sdi) != SR_OK) {
		sr_dbg("Could not set set gain.");
		return SR_ERR_IO;
	}

	return SR_OK;
}

SR_PRIV void configure_trigger(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc = sdi->priv;
	struct sr_serial_dev_inst *serial = sdi->conn;

	int channel;
	if(devc->trigger_channel->name == devc->channel_one_map->name)
		channel = 0;
	else
		channel = devc->trigger_channel->index;

	uint8_t *commands = g_malloc0(sizeof(uint8_t));
	*commands = ADC;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = CONFIGURE_TRIGGER;
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	*commands = (0 << 4) | (1 << channel);
	serial_write_blocking(serial,commands, 1, serial_timeout(serial, 1));
	uint16_t level = unscale(devc->trigger_channel,devc->trigger_voltage);
	serial_write_blocking(serial,&level, 2, serial_timeout(serial, 2));
	if(get_ack(sdi) != SR_OK)
		sr_dbg("Did not get ACK for configure trigger");
}

SR_PRIV float scale(const struct sr_channel *ch, uint16_t raw_value)
{
	sr_dbg("ln 433, in to scale raw value == %d", raw_value);
	struct channel_priv *cp = ch->priv;
	float slope = (float)((cp->max_input - cp->min_input) / cp->resolution * cp->gain);
	float intercept = (float)(cp->min_input/cp->gain);
	float x = slope * raw_value + intercept;
	sr_dbg("ln 438 scaled %d to voltage == %f", raw_value, x);
	return slope * raw_value + intercept;
}

SR_PRIV int unscale(const struct sr_channel *ch, double voltage)
{
	struct channel_priv *cp = ch->priv;
	double slope = (cp->max_input/cp->gain - cp->min_input/cp->gain) / cp->resolution;
	double intercept = cp->min_input/cp->gain;
	int level = (int)((voltage - intercept) / slope);
	if(level < 0)
		level =0;
	else if(level > cp->resolution )
		level = (int)cp->resolution;

	return level;
}

SR_PRIV int get_ack(const struct sr_dev_inst *sdi)
{
	struct sr_serial_dev_inst *serial = sdi->conn;
	int *buf = g_malloc0(1);
	serial_read_blocking(serial,buf,1, serial_timeout(serial,1));

	if(!(*buf & 0x01) || !(*buf)) {
		sr_dbg("Did not receive ACK or Received non ACK byte while waiting for ACK.");
		return SR_ERR_IO;
	}

	return SR_OK;
}