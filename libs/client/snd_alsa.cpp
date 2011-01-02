//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "snd_local.h"
#include <alsa/asoundlib.h>

// MACROS ------------------------------------------------------------------

#define BUFFER_SAMPLES		4096
#define SUBMISSION_CHUNK	BUFFER_SAMPLES / 2

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static snd_pcm_t*			pcm_handle;
static snd_pcm_hw_params_t*	hw_params;

static int					sample_bytes;
static int					buffer_bytes;

static QCvar*				sndbits;
static QCvar*				sndspeed;
static QCvar*				sndchannels;
static QCvar*				snddevice;

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
	if (!snddevice)
	{
		sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
		sndspeed = Cvar_Get("sndspeed", "0", CVAR_ARCHIVE);
		sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
		snddevice = Cvar_Get("s_alsaDevice", "default", CVAR_ARCHIVE);
	}

	int err = snd_pcm_open(&pcm_handle, snddevice->string,
		SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (err < 0)
	{
		GLog.Write("ALSA: cannot open device %s (%s)\n",
			snddevice->string, snd_strerror(err));
		return false;
	}

	err = snd_pcm_hw_params_malloc(&hw_params);
	if (err < 0)
	{
		GLog.Write("ALSA: cannot allocate hw params (%s)\n", snd_strerror(err));
		return false;
	}

	err = snd_pcm_hw_params_any(pcm_handle, hw_params);
	if (err < 0)
	{
		GLog.Write("ALSA: cannot init hw params(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0)
	{
		GLog.Write("ALSA: cannot set access(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	dma.samplebits = (int)sndbits->value;
	if (dma.samplebits != 8)
	{
		//try 16 by default
		dma.samplebits = 16;  //ensure this is set for other calculations
		err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_S16);
		if (err < 0)
		{
			GLog.Write("ALSA: 16 bit not supported, trying 8\n");
			dma.samplebits = 8;
		}
	}
	if (dma.samplebits == 8)
	{
		//or 8 if specifically asked to
		err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, SND_PCM_FORMAT_U8);
		if (err < 0)
		{
			GLog.Write("ALSA: cannot set format(%s)\n", snd_strerror(err));
			snd_pcm_hw_params_free(hw_params);
			return false;
		}
	}

	dma.speed = (int)sndspeed->value;
	if (dma.speed)
	{
		//try specified rate
		unsigned int r = dma.speed;
		int dir;
		err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &r, &dir);
		if (err < 0)
		{
			GLog.Write("ALSA: cannot set rate %d(%s)\n", r, snd_strerror(err));
		}
		else
		{
			//rate succeeded, but is perhaps slightly different
			if (dir != 0)
			{
				GLog.Write("ALSA: rate %d not supported, using %d\n", sndspeed->value, r);
			}
			dma.speed = r;
		}
	}
	if (!dma.speed)
	{
		//or all available ones
		for (int i = 0; i < sizeof(RATES) / sizeof(int); i++)
		{
			unsigned int r = RATES[i];
			int dir = 0;
			err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &r, &dir);
			if (err < 0)
			{
				GLog.Write("ALSA: cannot set rate %d(%s)\n", r, snd_strerror(err));
			}
			else
			{
				//rate succeeded, but is perhaps slightly different
				dma.speed = r;
				if (dir != 0)
				{
					GLog.Write("ALSA: rate %d not supported, using %d\n", RATES[i], r);
				}
				break;
			}
		}
	}
	if (!dma.speed)
	{
		//failed
		GLog.Write("ALSA: cannot set rate\n");
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	dma.channels = sndchannels->value;
	if (dma.channels < 1 || dma.channels > 2)
	{
		dma.channels = 2;  //ensure either stereo or mono
	}

	err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, dma.channels);
	if (err < 0)
	{
		GLog.Write("ALSA: cannot set channels %d(%s)\n",
			sndchannels->value, snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	snd_pcm_uframes_t p = BUFFER_SAMPLES / dma.channels;
	int dir;
	err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &p, &dir);
	if (err < 0)
	{
		GLog.Write("ALSA: cannot set period size (%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}
	else
	{
		//rate succeeded, but is perhaps slightly different
		if (dir != 0)
		{
			GLog.Write("ALSA: period %d not supported, using %d\n", (BUFFER_SAMPLES / dma.channels), p);
		}
	}

	err = snd_pcm_hw_params(pcm_handle, hw_params);
	if (err < 0)
	{
		//set params
		GLog.Write("ALSA: cannot set params(%s)\n", snd_strerror(err));
		snd_pcm_hw_params_free(hw_params);
		return false;
	}

	sample_bytes = dma.samplebits / 8;
	buffer_bytes = BUFFER_SAMPLES * sample_bytes;

	dma.buffer = new byte[buffer_bytes];  //allocate pcm frame buffer
	Com_Memset(dma.buffer, 0, buffer_bytes);

	dma.samplepos = 0;

	dma.samples = BUFFER_SAMPLES;
	dma.submission_chunk = SUBMISSION_CHUNK;

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
		GLog.Write("Sound not inizialized\n");
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

	int s = dma.samplepos * sample_bytes;
	void* start = &dma.buffer[s];

	int frames = dma.submission_chunk / dma.channels;

	int w = snd_pcm_writei(pcm_handle, start, frames);
	if (w < 0)
	{
		//write to card
		snd_pcm_prepare(pcm_handle);  //xrun occured
		return;
	}

	dma.samplepos += w * dma.channels;  //mark progress

	if (dma.samplepos >= dma.samples)
	{
		dma.samplepos = 0;  //wrap buffer
	}
}
