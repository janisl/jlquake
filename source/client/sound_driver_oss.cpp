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
#include "sound_local.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifdef __linux__
#include <linux/soundcard.h>
#endif
#ifdef __FreeBSD__
#include <sys/soundcard.h>
#endif

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int			audio_fd;
static int			snd_inited;

static QCvar*		sndbits;
static QCvar*		sndchannels;
static QCvar*		snddevice;

//	Some devices may work only with 48000
static int			tryrates[] = { 44100, 22050, 11025, 48000, 8000 };

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	SNDDMA_Init
//
//==========================================================================

bool SNDDMA_Init()
{
	if (snd_inited)
	{
		return true;
	}

	if (!snddevice)
	{
		sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
		sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
		snddevice = Cvar_Get("snddevice", "/dev/dsp", CVAR_ARCHIVE);
	}

	// open /dev/dsp, confirm capability to mmap, and get size of dma buffer
	if (!audio_fd)
	{
		audio_fd = open("/dev/dsp", O_RDWR);
		if (audio_fd < 0)
		{
			perror(snddevice->string);
			GLog.Write("Could not open /dev/dsp\n");
			return false;
		}
	}

	if (ioctl(audio_fd, SNDCTL_DSP_RESET, 0) < 0)//Not in Q3
	{
		perror(snddevice->string);
		GLog.Write("Could not reset %s\n", snddevice->string);
		close(audio_fd);
		return false;
	}

	int caps;
	if (ioctl(audio_fd, SNDCTL_DSP_GETCAPS, &caps) == -1)
	{
		perror(snddevice->string);
		GLog.Write("Sound driver too old\n");
		close(audio_fd);
		return false;
	}

	if (!(caps & DSP_CAP_TRIGGER) || !(caps & DSP_CAP_MMAP))
	{
		GLog.Write("Sorry but your soundcard can't do this\n");
		close(audio_fd);
		return 0;
	}

	//	SNDCTL_DSP_GETOSPACE moved to be called later

	// set sample bits & speed
	dma.samplebits = (int)sndbits->value;
	if (dma.samplebits != 16 && dma.samplebits != 8)
	{
		int fmt;
		ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &fmt);
		if (fmt & AFMT_S16_LE)
		{
			dma.samplebits = 16;
		}
		else if (fmt & AFMT_U8)
		{
			dma.samplebits = 8;
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

	if (!dma.speed)
	{
		int i;
		for (i = 0; i < sizeof(tryrates) / 4; i++)
		{
			if (!ioctl(audio_fd, SNDCTL_DSP_SPEED, &tryrates[i]))
			{
				break;
			}
		}
		dma.speed = tryrates[i];
	}

	dma.channels = (int)sndchannels->value;
	if (dma.channels < 1 || dma.channels > 2)
	{
		dma.channels = 2;
	}

	//	mmap() call moved forward

	int tmp = 0;
	if (dma.channels == 2)
	{
		tmp = 1;
	}
	if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &tmp) < 0)
	{
		perror(snddevice->string);
		GLog.Write("Could not set %s to stereo=%d", snddevice->string, dma.channels);
		close(audio_fd);
		return 0;
	}
	if (tmp)
	{
		dma.channels = 2;
	}
	else
	{
		dma.channels = 1;
	}

	if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &dma.speed) < 0)
	{
		perror(snddevice->string);
		GLog.Write("Could not set %s speed to %d", snddevice->string, dma.speed);
		close(audio_fd);
		return 0;
	}

	if (dma.samplebits == 16)
	{
		tmp = AFMT_S16_LE;
		if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &tmp) < 0)
		{
			perror(snddevice->string);
			GLog.Write("Could not support 16-bit data.  Try 8-bit.\n");
			close(audio_fd);
			return 0;
		}
	}
	else if (dma.samplebits == 8)
	{
		tmp = AFMT_U8;
		if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &tmp) < 0)
		{
			perror(snddevice->string);
			GLog.Write("Could not support 8-bit data.\n");
			close(audio_fd);
			return 0;
		}
	}
	else
	{
		perror(snddevice->string);
		GLog.Write("%d-bit sound not supported.", dma.samplebits);
		close(audio_fd);
		return 0;
	}

	audio_buf_info info;
	if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
	{   
		perror("GETOSPACE");
		GLog.Write("Um, can't do GETOSPACE?\n");
		close(audio_fd);
		return 0;
	}

	dma.samples = info.fragstotal * info.fragsize / (dma.samplebits/8);
	dma.submission_chunk = 1;

	// memory map the dma buffer

	// TTimo 2001/10/08 added PROT_READ to the mmap
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=371
	// checking Alsa bug, doesn't allow dma alloc with PROT_READ?

	if (!dma.buffer)
	{
		dma.buffer = (byte*)mmap(NULL, info.fragstotal * info.fragsize,
			PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, audio_fd, 0);
	}

	if (dma.buffer == MAP_FAILED)
	{
		GLog.Write("Could not mmap dma buffer PROT_WRITE|PROT_READ\n");
		GLog.Write("trying mmap PROT_WRITE (with associated better compatibility / less performance code)\n");
		dma.buffer = (byte*)mmap(NULL, info.fragstotal * info.fragsize,
			PROT_WRITE, MAP_FILE | MAP_SHARED, audio_fd, 0);
		// NOTE TTimo could add a variable to force using regular memset on systems that are known to be safe
		s_use_custom_memset = true;
	}

	if (dma.buffer == MAP_FAILED)
	{
		perror(snddevice->string);
		GLog.Write("Could not mmap %s\n", snddevice->string);
		close(audio_fd);
		return 0;
	}

	// toggle the trigger & start her up

	tmp = 0;
	if (ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp) < 0)
	{
		perror(snddevice->string);
		GLog.Write("Could not toggle.\n");
		close(audio_fd);
		return 0;
	}
	tmp = PCM_ENABLE_OUTPUT;
	if (ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp) < 0)
	{
		perror(snddevice->string);
		GLog.Write("Could not toggle.\n");
		close(audio_fd);
		return 0;
	}

	dma.samplepos = 0;

	snd_inited = 1;
	return true;
}

//==========================================================================
//
//	SNDDMA_Shutdown
//
//==========================================================================

void SNDDMA_Shutdown()
{
}

//==========================================================================
//
//	SNDDMA_GetDMAPos
//
//==========================================================================

int SNDDMA_GetDMAPos()
{
	if (!snd_inited)
	{
		return 0;
	}

	count_info count;
	if (ioctl(audio_fd, SNDCTL_DSP_GETOPTR, &count) == -1)
	{
		perror(snddevice->string);
		GLog.Write("Uh, sound dead.\n");
		close(audio_fd);
		snd_inited = 0;
		return 0;
	}
	dma.samplepos = count.ptr / (dma.samplebits / 8);
	return dma.samplepos;
}

//==========================================================================
//
//	SNDDMA_Submit
//
//	Send sound to device if buffer isn't really the dma buffer
//
//==========================================================================

void SNDDMA_Submit()
{
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
