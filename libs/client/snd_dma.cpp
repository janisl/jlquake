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
sfx_t*		sfxHash[LOOP_HASH];

fileHandle_t s_backgroundFile;
wavinfo_t	s_backgroundInfo;
int			s_backgroundSamples;
char		s_backgroundLoop[MAX_QPATH];

sfx_t		*ambient_sfx[BSP29_NUM_AMBIENTS];

int			s_registration_sequence;
bool		s_registering;

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

//==========================================================================
//
//	S_HashSFXName
//
//	return a hash value for the sfx name
//
//==========================================================================

int S_HashSFXName(const char* Name)
{
	int Hash = 0;
	int i = 0;
	while (Name[i] != '\0')
	{
		char Letter = QStr::ToLower(Name[i]);
		if (Letter =='.')
		{
			break;				// don't include extension
		}
		if (Letter =='\\')
		{
			Letter = '/';		// damn path names
		}
		Hash += (long)(Letter) * (i + 119);
		i++;
	}
	Hash &= (LOOP_HASH - 1);
	return Hash;
}

//==========================================================================
//
//	S_FindName
//
//	Will allocate a new sfx if it isn't found
//
//==========================================================================

sfx_t* S_FindName(const char* Name, bool Create)
{
	if (!Name)
	{
		throw QException("S_FindName: NULL\n");
	}
	if (!Name[0])
	{
		throw QException("S_FindName: empty name\n");
	}

	if (QStr::Length(Name) >= MAX_QPATH)
	{
		throw QException(va("Sound name too long: %s", Name));
	}

	int Hash = S_HashSFXName(Name);

	sfx_t* Sfx = sfxHash[Hash];
	// see if already loaded
	while (Sfx)
	{
		if (!QStr::ICmp(Sfx->Name, Name))
		{
			return Sfx;
		}
		Sfx = Sfx->HashNext;
	}

	if (!Create)
	{
		return NULL;
	}

	//	Find a free sfx.
	int i;
	for (i = 0; i < s_numSfx; i++)
	{
		if (!s_knownSfx[i].Name[0])
		{
			break;
		}
	}

	if (i == s_numSfx)
	{
		if (s_numSfx == MAX_SFX)
		{
			throw QException("S_FindName: out of sfx_t");
		}
		s_numSfx++;
	}

	Sfx = &s_knownSfx[i];
	Com_Memset(Sfx, 0, sizeof(*Sfx));
	QStr::Cpy(Sfx->Name, Name);
	Sfx->RegistrationSequence = s_registration_sequence;

	Sfx->HashNext = sfxHash[Hash];
	sfxHash[Hash] = Sfx;

	return Sfx;
}

//==========================================================================
//
//	S_AliasName
//
//==========================================================================

sfx_t* S_AliasName(const char* AliasName, const char* TrueName)
{

	char* S = new char[MAX_QPATH];
	QStr::Cpy(S, TrueName);

	// find a free sfx
	int i;
	for (i = 0; i < s_numSfx; i++)
	{
		if (!s_knownSfx[i].Name[0])
		{
			break;
		}
	}

	if (i == s_numSfx)
	{
		if (s_numSfx == MAX_SFX)
		{
			throw QException("S_FindName: out of sfx_t");
		}
		s_numSfx++;
	}

	sfx_t* Sfx = &s_knownSfx[i];
	Com_Memset(Sfx, 0, sizeof(*Sfx));
	QStr::Cpy(Sfx->Name, AliasName);
	Sfx->RegistrationSequence = s_registration_sequence;
	Sfx->TrueName = S;

	int Hash = S_HashSFXName(AliasName);
	Sfx->HashNext = sfxHash[Hash];
	sfxHash[Hash] = Sfx;

	return Sfx;
}

//==========================================================================
//
//	S_BeginRegistration
//
//==========================================================================

void S_BeginRegistration()
{
	if (GGameType & GAME_Quake2)
	{
		s_registration_sequence++;
		s_registering = true;
	}

	if (GGameType & GAME_Quake3)
	{
		s_soundMuted = false;		// we can play again

		if (s_numSfx == 0)
		{
			s_numSfx = 0;
			Com_Memset(s_knownSfx, 0, sizeof(s_knownSfx));
			Com_Memset(sfxHash, 0, sizeof(sfx_t*) * LOOP_HASH);

			S_RegisterSound("sound/feedback/hit.wav");		// changed to a sound in baseq3
		}
	}
}

//==========================================================================
//
//	S_RegisterSound
//
//	Creates a default buzz sound if the file can't be loaded
//
//==========================================================================

sfxHandle_t S_RegisterSound(const char* Name)
{
	if (!s_soundStarted)
	{
		return 0;
	}

	if (QStr::Length(Name) >= MAX_QPATH)
	{
		GLog.Write("Sound name exceeds MAX_QPATH\n");
		return 0;
	}

	sfx_t* Sfx = S_FindName(Name);
	Sfx->RegistrationSequence = s_registration_sequence;
	if (Sfx->Data)
	{
		if ((GGameType & GAME_Quake3) && Sfx->DefaultSound)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: could not find %s - using default\n", Sfx->Name);
			return 0;
		}
		return Sfx - s_knownSfx;
	}

	Sfx->InMemory = false;

	if (!(GGameType & GAME_Quake2) || !s_registering)
		S_LoadSound(Sfx);

	if ((GGameType & GAME_Quake3) && Sfx->DefaultSound)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: could not find %s - using default\n", Sfx->Name);
		return 0;
	}

	return Sfx - s_knownSfx;
}

//==========================================================================
//
//	S_EndRegistration
//
//==========================================================================

void S_EndRegistration()
{
	if (GGameType & GAME_Quake2)
	{
		//	Free any sounds not from this registration sequence
		sfx_t* Sfx = s_knownSfx;
		for (int i = 0; i < s_numSfx; i++, Sfx++)
		{
			if (!Sfx->Name[0])
			{
				continue;
			}
			if (Sfx->RegistrationSequence != s_registration_sequence)
			{
				//	Don't need this sound
				if (Sfx->Data)	// it is possible to have a leftover
				{
					delete[] Sfx->Data;	// from a server that didn't finish loading
				}
				if (Sfx->TrueName)
				{
					delete[] Sfx->TrueName;
				}
				Com_Memset(Sfx, 0, sizeof(*Sfx));
			}
		}

		//	Need to rehash remaining sounds.
		Com_Memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

		//	Load everything in
		Sfx = s_knownSfx;
		for (int i = 0; i < s_numSfx; i++, Sfx++)
		{
			if (!Sfx->Name[0])
			{
				continue;
			}

			S_LoadSound(Sfx);

			int Hash = S_HashSFXName(Sfx->Name);
			Sfx->HashNext = sfxHash[Hash];
			sfxHash[Hash] = Sfx;
		}

		s_registering = false;
	}
}
