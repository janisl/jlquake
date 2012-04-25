//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"
#include <alsa/asoundlib.h>

// MACROS ------------------------------------------------------------------

#define BUFFER_SAMPLES      4096
#define SUBMISSION_CHUNK    BUFFER_SAMPLES / 2

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static snd_pcm_t* pcm_handle;
static snd_pcm_hw_params_t* hw_params;

static int sample_bytes;
static int buffer_bytes;

static Cvar* snddevice;

//	The sample rates which will be attempted.
static int RATES[] =
{
	44100, 22050, 11025, 48000, 8000
};

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	SNDDMA_Init
//
//	Initialize ALSA pcm device.
//
//==========================================================================

bool SNDDMA_Init()
{
	snddevice = Cvar_Get("s_alsaDevice", "default", CVAR_ARCHIVE);

	int err = snd_pcm_open(&pcm_handle, snddevice->string,
		SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (err < 0)
	{
		Log::write("ALSA: cannot open device %s (%s)\n",
			snddevice->string, snd_strerror(err));
		return false;
	}

	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0)
	{
		Log::write("ALSA: cannot allocate hw params (%s)\n", snd_strerror(err));
		return false;
	}

	err = snd_pcm_hw_params_any(pcm_handle, hw_params);
	if (err < 0)
	{
		Log::write("ALSA: cannot init hw params(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0)
	{
		Log::write("ALSA: cannot set access(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	dma.samplebits = s_bits->integer;
	if (dma.samplebits != 8)
	{
		//try 16 by default
		dma.samplebits = 16;	//ensure this is set for other calculations
		err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16);
		if (err < 0)
		{
			Log::write("ALSA: 16 bit not supported, trying 8\n");
			dma.samplebits = 8;
		}
	}
	if (dma.samplebits == 8)
	{
		//or 8 if specifically asked to
		err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_U8);
		if (err < 0)
		{
			Log::write("ALSA: cannot set format(%s)\n", snd_strerror(err));
			snd_pcm_hw_params_free(hw_params);
			return false;
		}
	}

	if (s_khz->integer == 44)
	{
		dma.speed = 44100;
	}
	else if (s_khz->integer == 22)
	{
		dma.speed = 22050;
	}
	else if (s_khz->integer == 11)
	{
		dma.speed = 11025;
	}
	else
	{
		dma.speed = 0;
	}

	if (dma.speed)
	{
		//try specified rate
		unsigned int r = dma.speed;
		int dir;
		err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &r, &dir);
		if (err < 0)
		{
			Log::write("ALSA: cannot set rate %d(%s)\n", r, snd_strerror(err));
			dma.speed = 0;
		}
		else
		{
			//rate succeeded, but is perhaps slightly different
			if (dir != 0 && dma.speed != (int)r)
			{
				Log::write("ALSA: rate %d not supported, using %d\n", dma.speed, r);
			}
			dma.speed = r;
		}
	}
	if (!dma.speed)
	{
		//or all available ones
		for (int i = 0; i < (int)(sizeof(RATES) / sizeof(int)); i++)
		{
			unsigned int r = RATES[i];
			int dir = 0;
			err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &r, &dir);
			if (err < 0)
			{
				Log::write("ALSA: cannot set rate %d(%s)\n", r, snd_strerror(err));
			}
			else
			{
				//rate succeeded, but is perhaps slightly different
				dma.speed = r;
				if (dir != 0)
				{
					Log::write("ALSA: rate %d not supported, using %d\n", RATES[i], r);
				}
				break;
			}
		}
	}
	if (!dma.speed)
	{
		//failed
		Log::write("ALSA: cannot set rate\n");
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	dma.channels = s_channels_cv->integer;
	if (dma.channels < 1 || dma.channels > 2)
	{
		dma.channels = 2;	//ensure either stereo or mono
	}

	err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, dma.channels);
	if (err < 0)
	{
		Log::write("ALSA: cannot set channels %d(%s)\n",
			s_channels_cv->integer, snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	snd_pcm_uframes_t p = BUFFER_SAMPLES / dma.channels;
	err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &p);
	if (err < 0)
	{
		Log::write("ALSA: Unable to set buffer size %li: %s\n", BUFFER_SAMPLES / dma.channels, snd_strerror(err));
		//return false;
	}

	int dir;
	err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &p, &dir);
	if (err < 0)
	{
		Log::write("ALSA: cannot set period size (%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}
	else
	{
		//rate succeeded, but is perhaps slightly different
		if (dir != 0)
		{
			Log::write("ALSA: period %d not supported, using %d\n", (BUFFER_SAMPLES / dma.channels), p);
		}
	}

	err = snd_pcm_hw_params(pcm_handle, hw_params);
	if (err < 0)
	{
		//set params
		Log::write("ALSA: cannot set params(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	sample_bytes = dma.samplebits / 8;
	buffer_bytes = BUFFER_SAMPLES * sample_bytes;

	dma.buffer = new byte[buffer_bytes];	//allocate pcm frame buffer
	Com_Memset(dma.buffer, 0, buffer_bytes);

	dma.samplepos = 0;

	dma.samples = BUFFER_SAMPLES;
	dma.submission_chunk = p * dma.channels;

	snd_pcm_prepare(pcm_handle);

	return true;
}

//==========================================================================
//
//	SNDDMA_Shutdown
//
//	Closes the ALSA pcm device and frees the dma buffer.
//
//==========================================================================

void SNDDMA_Shutdown()
{
	if (dma.buffer)
	{
		snd_pcm_drop(pcm_handle);
		snd_pcm_close(pcm_handle);
		delete[] dma.buffer;
		dma.buffer = NULL;
	}
}

//==========================================================================
//
//	SNDDMA_GetDMAPos
//
//	Returns the current sample position, if sound is running.
//
//==========================================================================

int SNDDMA_GetDMAPos()
{
	if (!dma.buffer)
	{
		Log::write("Sound not inizialized\n");
		return 0;
	}

//	snd_pcm_sframes_t delay;
//	snd_pcm_delay(pcm_handle, &delay);
//	int ret = dma.samplepos - delay;
//	return (Sys_Milliseconds() * dma.speed / 1000) % dma.samples;
	return dma.samplepos;
}

//==========================================================================
//
//	SNDDMA_BeginPainting
//
//	Callback provided by the engine in case we need it.  We don't.
//
//==========================================================================

void SNDDMA_BeginPainting()
{
}

//==========================================================================
//
//	SNDDMA_Submit
//
//	Writes the dma buffer to the ALSA pcm device.
//
//==========================================================================

void SNDDMA_Submit()
{
	if (!dma.buffer)
	{
		return;
	}

	int Submitted = 0;
	do
	{
		int s = dma.samplepos * sample_bytes;
		void* start = &dma.buffer[s];

		int frames = dma.submission_chunk / dma.channels;
		if (dma.samplepos + frames * dma.channels > dma.samples)
		{
			frames = (dma.samples - dma.samplepos) / dma.channels;
		}
		if (Submitted + frames >= dma.samples / dma.channels)
		{
			frames = dma.samples / dma.channels - Submitted - 1;
		}

		int w = snd_pcm_writei(pcm_handle, start, frames);
		if (w == -EAGAIN)
		{
			break;
		}
		if (w < 0)
		{
			//write to card
			snd_pcm_prepare(pcm_handle);	//xrun occured
			return;
		}

		dma.samplepos += w * dma.channels;	//mark progress

		if (dma.samplepos >= dma.samples)
		{
			dma.samplepos = 0;	//wrap buffer
		}
		Submitted += w;
	}
	while (Submitted < (dma.samples / dma.channels) - 1);
}
