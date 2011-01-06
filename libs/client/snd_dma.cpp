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

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

dma_t		dma;

int			s_soundtime;	// sample PAIRS
int   		s_paintedtime; 	// sample PAIRS

QCvar*		s_volume;
QCvar*		s_testsound;
QCvar*		s_khz;
QCvar*		s_show;
QCvar*		s_mixahead;
QCvar		*s_mixPreStep;
QCvar		*s_musicVolume;
QCvar		*s_separation;
QCvar		*s_doppler;
QCvar* ambient_level;
QCvar* ambient_fade;
QCvar* snd_noextraupdate;

channel_t   s_channels[MAX_CHANNELS];
channel_t   loop_channels[MAX_CHANNELS];
int			numLoopChannels;

playsound_t	s_pendingplays;

bool		s_use_custom_memset = false;

int						s_rawend;
portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

int			s_soundStarted;
bool		s_soundMuted;

int			listener_number;
vec3_t		listener_origin;
vec3_t		listener_axis[3];

sfx_t		s_knownSfx[MAX_SFX];
int			s_numSfx = 0;

fileHandle_t s_backgroundFile;
wavinfo_t	s_backgroundInfo;
int			s_backgroundSamples;
char		s_backgroundLoop[MAX_QPATH];

sfx_t		*ambient_sfx[BSP29_NUM_AMBIENTS];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Snd_Memset
//
//==========================================================================

// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=371 
void Snd_Memset(void* dest, const int val, const size_t count)
{
	if (!s_use_custom_memset)
	{
		Com_Memset(dest, val, count);
		return;
	}
	int iterate = count / sizeof(int);
	int* pDest = (int*)dest;
	for (int i = 0; i < iterate; i++)
	{
		pDest[i] = val;
	}
}

//==========================================================================
//
//	S_SoundInfo_f
//
//==========================================================================

void S_SoundInfo_f()
{
	GLog.Write("----- Sound Info -----\n");
	if (!s_soundStarted)
	{
		GLog.Write("sound system not started\n");
	}
	else
	{
		if (s_soundMuted)
		{
			GLog.Write("sound system is muted\n");
		}

		GLog.Write("%5d stereo\n", dma.channels - 1);
		GLog.Write("%5d samples\n", dma.samples);
		GLog.Write("%5d samplebits\n", dma.samplebits);
		GLog.Write("%5d submission_chunk\n", dma.submission_chunk);
		GLog.Write("%5d speed\n", dma.speed);
		GLog.Write("0x%x dma buffer\n", dma.buffer);
		if (s_backgroundFile)
		{
			GLog.Write("Background file: %s\n", s_backgroundLoop);
		}
		else
		{
			GLog.Write("No background file.\n" );
		}
	}
	GLog.Write("----------------------\n" );
}
