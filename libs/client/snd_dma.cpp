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

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define SOUND_FULLVOLUME		80

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

int S_GetClientFrameCount();

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
QCvar		*s_doppler;
QCvar* ambient_level;
QCvar* ambient_fade;
QCvar* snd_noextraupdate;

channel_t   s_channels[MAX_CHANNELS];
channel_t   loop_channels[MAX_CHANNELS];
int			numLoopChannels;
channel_t*	freelist = NULL;

playsound_t	s_pendingplays;
playsound_t	s_playsounds[MAX_PLAYSOUNDS];
playsound_t	s_freeplays;

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

loopSound_t	loopSounds[MAX_LOOPSOUNDS];

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
//	S_ChannelFree
//
//==========================================================================

void S_ChannelFree(channel_t* v)
{
	v->sfx = NULL;
	*(channel_t**)v = freelist;
	freelist = (channel_t*)v;
}

//==========================================================================
//
//	S_ChannelMalloc
//
//==========================================================================

channel_t* S_ChannelMalloc()
{
	channel_t *v;
	if (freelist == NULL)
	{
		return NULL;
	}
	v = freelist;
	freelist = *(channel_t**)freelist;
	v->allocTime = Com_Milliseconds();
	return v;
}

//==========================================================================
//
//	S_ChannelSetup
//
//==========================================================================

void S_ChannelSetup()
{
	channel_t *p, *q;

	// clear all the sounds so they don't
	Com_Memset(s_channels, 0, sizeof(s_channels));

	p = s_channels;;
	q = p + MAX_CHANNELS;
	while (--q > p)
	{
		*(channel_t**)q = q - 1;
	}

	*(channel_t**)q = NULL;
	freelist = p + MAX_CHANNELS - 1;
	GLog.DWrite("Channel memory manager started\n");
}

//**************************************************************************
//	Registration of sounds
//**************************************************************************

//==========================================================================
//
//	S_HashSFXName
//
//	return a hash value for the sfx name
//
//==========================================================================

static int S_HashSFXName(const char* Name)
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

//**************************************************************************
//	Background music functions
//**************************************************************************

//==========================================================================
//
//	S_ByteSwapRawSamples
//
//	If raw data has been loaded in little endien binary form, this must be done.
//	If raw data was calculated, as with ADPCM, this should not be called.
//
//==========================================================================

void S_ByteSwapRawSamples(int samples, int width, int s_channels, const byte *data)
{
	int		i;

	if (width != 2)
	{
		return;
	}
	if (LittleShort(256) == 256)
	{
		return;
	}

	if (s_channels == 2)
	{
		samples <<= 1;
	}
	for (i = 0; i < samples; i++)
	{
		((short*)data)[i] = LittleShort(((short*)data)[i]);
	}
}

//==========================================================================
//
//	S_RawSamples
//
//	Music and cinematic streaming
//
//==========================================================================

void S_RawSamples(int samples, int rate, int width, int channels, const byte *data, float volume)
{
	int		i;
	int		src, dst;
	float	scale;
	int		intVolume;

	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	intVolume = 256 * volume;

	if (s_rawend < s_soundtime)
	{
		GLog.DWrite("S_RawSamples: resetting minimum: %i < %i\n", s_rawend, s_soundtime);
		s_rawend = s_soundtime;
	}

	scale = (float)rate / dma.speed;

//Com_Printf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend);
	if (channels == 2 && width == 2)
	{
		if (scale == 1.0)
		{	// optimized case
			for (i=0 ; i<samples ; i++)
			{
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left = ((short*)data)[i * 2] * intVolume;
				s_rawsamples[dst].right = ((short*)data)[i * 2 + 1] * intVolume;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left = ((short *)data)[src*2] * intVolume;
				s_rawsamples[dst].right = ((short *)data)[src*2+1] * intVolume;
			}
		}
	}
	else if (channels == 1 && width == 2)
	{
		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend&(MAX_RAW_SAMPLES-1);
			s_rawend++;
			s_rawsamples[dst].left = ((short *)data)[src] * intVolume;
			s_rawsamples[dst].right = ((short *)data)[src] * intVolume;
		}
	}
	else if (channels == 2 && width == 1)
	{
		intVolume *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend&(MAX_RAW_SAMPLES-1);
			s_rawend++;
			s_rawsamples[dst].left = ((char *)data)[src*2] * intVolume;
			s_rawsamples[dst].right = ((char *)data)[src*2+1] * intVolume;
		}
	}
	else if (channels == 1 && width == 1)
	{
		intVolume *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend&(MAX_RAW_SAMPLES-1);
			s_rawend++;
			s_rawsamples[dst].left = (((byte *)data)[src]-128) * intVolume;
			s_rawsamples[dst].right = (((byte *)data)[src]-128) * intVolume;
		}
	}

	if (s_rawend > s_soundtime + MAX_RAW_SAMPLES)
	{
		GLog.DWrite("S_RawSamples: overflowed %i > %i\n", s_rawend, s_soundtime);
	}
}

//==========================================================================
//
//	FGetLittleLong
//
//==========================================================================

static int FGetLittleLong(fileHandle_t f)
{
	int		v;

	FS_Read(&v, sizeof(v), f);

	return LittleLong(v);
}

//==========================================================================
//
//	FGetLittleShort
//
//==========================================================================

static int FGetLittleShort(fileHandle_t f)
{
	short	v;

	FS_Read(&v, sizeof(v), f);

	return LittleShort(v);
}

//==========================================================================
//
//	FGetLittleShort
//
//	Returns the length of the data in the chunk, or 0 if not found
//
//==========================================================================

static int S_FindWavChunk(fileHandle_t f, const char* Chunk)
{
	char	Name[5];

	Name[4] = 0;
	int R = FS_Read(Name, 4, f);
	if (R != 4)
	{
		return 0;
	}
	int Len = FGetLittleLong(f);
	if (Len < 0 || Len > 0xfffffff)
	{
		Len = 0;
		return 0;
	}
	Len = (Len + 1 ) & ~1;		// pad to word boundary

	if (QStr::Cmp(Name, Chunk))
	{
		return 0;
	}

	return Len;
}

//==========================================================================
//
//	S_StartBackgroundTrack
//
//==========================================================================

void S_StartBackgroundTrack(const char* intro, const char* loop)
{
	int		len;
	char	dump[16];
	char	name[MAX_QPATH];

	if (!intro)
	{
		intro = "";
	}
	if (!loop || !loop[0])
	{
		loop = intro;
	}
	GLog.DWrite("S_StartBackgroundTrack( %s, %s )\n", intro, loop);

	QStr::NCpyZ(name, intro, sizeof(name) - 4);
	QStr::DefaultExtension(name, sizeof(name), ".wav");

	if (!intro[0])
	{
		return;
	}

	QStr::NCpyZ(s_backgroundLoop, loop, sizeof(s_backgroundLoop));

	// close the background track, but DON'T reset s_rawend
	// if restarting the same back ground track
	if (s_backgroundFile)
	{
		FS_FCloseFile(s_backgroundFile);
		s_backgroundFile = 0;
	}

	//
	// open up a wav file and get all the info
	//
	FS_FOpenFileRead(name, &s_backgroundFile, true);
	if (!s_backgroundFile)
	{
		if (GGameType & GAME_Quake3)
			GLog.Write(S_COLOR_YELLOW "WARNING: couldn't open music file %s\n", name);
		else
			GLog.Write("WARNING: couldn't open music file %s\n", name);
		return;
	}

	// skip the riff wav header

	FS_Read(dump, 12, s_backgroundFile);

	if (!S_FindWavChunk(s_backgroundFile, "fmt "))
	{
		GLog.Write("No fmt chunk in %s\n", name);
		FS_FCloseFile(s_backgroundFile);
		s_backgroundFile = 0;
		return;
	}

	// save name for soundinfo
	s_backgroundInfo.format = FGetLittleShort(s_backgroundFile);
	s_backgroundInfo.channels = FGetLittleShort(s_backgroundFile);
	s_backgroundInfo.rate = FGetLittleLong(s_backgroundFile);
	FGetLittleLong( s_backgroundFile);
	FGetLittleShort(s_backgroundFile);
	s_backgroundInfo.width = FGetLittleShort(s_backgroundFile) / 8;

	if (s_backgroundInfo.format != WAV_FORMAT_PCM)
	{
		FS_FCloseFile(s_backgroundFile);
		s_backgroundFile = 0;
		GLog.Write("Not a microsoft PCM format wav: %s\n", name);
		return;
	}

	if (s_backgroundInfo.channels != 2 || s_backgroundInfo.rate < 22050)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: music file %s is not 22k or higher stereo\n", name);
	}

	if (( len = S_FindWavChunk(s_backgroundFile, "data")) == 0)
	{
		FS_FCloseFile(s_backgroundFile);
		s_backgroundFile = 0;
		GLog.Write("No data chunk in %s\n", name);
		return;
	}

	s_backgroundInfo.samples = len / (s_backgroundInfo.width * s_backgroundInfo.channels);

	s_backgroundSamples = s_backgroundInfo.samples;
}

//==========================================================================
//
//	S_StopBackgroundTrack
//
//==========================================================================

void S_StopBackgroundTrack()
{
	if (!s_backgroundFile)
	{
		return;
	}
	FS_FCloseFile(s_backgroundFile);
	s_backgroundFile = 0;
	s_rawend = 0;
}

//==========================================================================
//
//	S_UpdateBackgroundTrack
//
//==========================================================================

void S_UpdateBackgroundTrack()
{
	int		bufferSamples;
	int		fileSamples;
	byte	raw[30000];		// just enough to fit in a mac stack frame
	int		fileBytes;
	int		r;
	static	float	musicVolume = 0.5f;

	if (!s_backgroundFile)
	{
		return;
	}

	// graeme see if this is OK
	musicVolume = (musicVolume + (s_musicVolume->value * 2)) / 4.0f;

	// don't bother playing anything if musicvolume is 0
	if (musicVolume <= 0)
	{
		return;
	}

	// see how many samples should be copied into the raw buffer
	if (s_rawend < s_soundtime)
	{
		s_rawend = s_soundtime;
	}

	while (s_rawend < s_soundtime + MAX_RAW_SAMPLES)
	{
		bufferSamples = MAX_RAW_SAMPLES - (s_rawend - s_soundtime);

		// decide how much data needs to be read from the file
		fileSamples = bufferSamples * s_backgroundInfo.rate / dma.speed;
		if (!fileSamples)
		{
			//	Could happen if dma.speed > s_backgroundInfo.rate and
			// bufferSamples is 1
			break;
		}

		// don't try and read past the end of the file
		if (fileSamples > s_backgroundSamples)
		{
			fileSamples = s_backgroundSamples;
		}

		// our max buffer size
		fileBytes = fileSamples * (s_backgroundInfo.width * s_backgroundInfo.channels);
		if (fileBytes > sizeof(raw))
		{
			fileBytes = sizeof(raw);
			fileSamples = fileBytes / (s_backgroundInfo.width * s_backgroundInfo.channels);
		}

		r = FS_Read(raw, fileBytes, s_backgroundFile);
		if (r != fileBytes)
		{
			GLog.Write("StreamedRead failure on music track\n");
			S_StopBackgroundTrack();
			return;
		}

		// byte swap if needed
		S_ByteSwapRawSamples(fileSamples, s_backgroundInfo.width, s_backgroundInfo.channels, raw);

		// add to raw buffer
		S_RawSamples(fileSamples, s_backgroundInfo.rate, 
			s_backgroundInfo.width, s_backgroundInfo.channels, raw, musicVolume);

		s_backgroundSamples -= fileSamples;
		if (!s_backgroundSamples)
		{
			// loop
			if (s_backgroundLoop[0])
			{
				FS_FCloseFile(s_backgroundFile);
				s_backgroundFile = 0;
				S_StartBackgroundTrack(s_backgroundLoop, s_backgroundLoop);
				if (!s_backgroundFile)
				{
					return;		// loop failed to restart
				}
			}
			else
			{
				s_backgroundFile = 0;
				return;
			}
		}
	}
}

//**************************************************************************
//	Commands
//**************************************************************************

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
//	S_Music_f
//
//==========================================================================

void S_Music_f()
{
	int c = Cmd_Argc();

	if (c == 2)
	{
		S_StartBackgroundTrack(Cmd_Argv(1), Cmd_Argv(1));
		s_backgroundLoop[0] = 0;
	}
	else if (c == 3)
	{
		S_StartBackgroundTrack(Cmd_Argv(1), Cmd_Argv(2));
	}
	else
	{
		GLog.Write("music <musicfile> [loopfile]\n");
		return;
	}
}

//==========================================================================
//
//	S_UpdateEntityPosition
//
//	Let the sound system know where an entity currently is.
//
//==========================================================================

void S_UpdateEntityPosition(int EntityNum, const vec3_t Origin)
{
	if (EntityNum < 0 || EntityNum > MAX_LOOPSOUNDS)
	{
		QDropException(va("S_UpdateEntityPosition: bad entitynum %i", EntityNum));
	}
	VectorCopy(Origin, loopSounds[EntityNum].origin);
}

//==========================================================================
//
//	S_StopLoopingSound
//
//==========================================================================

void S_StopLoopingSound(int EntityNum)
{
	loopSounds[EntityNum].active = false;
//	loopSounds[EntityNum].sfx = NULL;
	loopSounds[EntityNum].kill = false;
}

//==========================================================================
//
//	S_ClearLoopingSounds
//
//==========================================================================

void S_ClearLoopingSounds(bool KillAll)
{
	for (int i = 0; i < MAX_LOOPSOUNDS; i++)
	{
		if (KillAll || loopSounds[i].kill == true || (loopSounds[i].sfx && loopSounds[i].sfx->Length == 0))
		{
			loopSounds[i].kill = false;
			S_StopLoopingSound(i);
		}
	}
	numLoopChannels = 0;
}

//==========================================================================
//
//	S_AddLoopingSound
//
//	Called during entity generation for a frame
//	Include velocity in case I get around to doing doppler...
//
//==========================================================================

void S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle)
{
	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	if (sfxHandle < 0 || sfxHandle >= s_numSfx)
	{
		if (GGameType & GAME_Quake3)
		{
			GLog.Write(S_COLOR_YELLOW "S_AddLoopingSound: handle %i out of range\n", sfxHandle);
		}
		else
		{
			GLog.Write("S_AddLoopingSound: handle %i out of range\n", sfxHandle);
		}
		return;
	}

	sfx_t* sfx = &s_knownSfx[sfxHandle];

	if (sfx->InMemory == false)
	{
		S_LoadSound(sfx);
	}

	if (!sfx->Length)
	{
		QDropException(va("%s has length 0", sfx->Name));
	}

	VectorCopy(origin, loopSounds[entityNum].origin);
	VectorCopy(velocity, loopSounds[entityNum].velocity);
	loopSounds[entityNum].active = true;
	loopSounds[entityNum].kill = true;
	loopSounds[entityNum].doppler = false;
	loopSounds[entityNum].oldDopplerScale = 1.0;
	loopSounds[entityNum].dopplerScale = 1.0;
	loopSounds[entityNum].sfx = sfx;

	if (s_doppler->integer && VectorLengthSquared(velocity) > 0.0)
	{
		vec3_t	out;
		float	lena, lenb;

		loopSounds[entityNum].doppler = true;
		lena = DistanceSquared(loopSounds[listener_number].origin, loopSounds[entityNum].origin);
		VectorAdd(loopSounds[entityNum].origin, loopSounds[entityNum].velocity, out);
		lenb = DistanceSquared(loopSounds[listener_number].origin, out);
		if ((loopSounds[entityNum].framenum + 1) != S_GetClientFrameCount())
		{
			loopSounds[entityNum].oldDopplerScale = 1.0;
		}
		else
		{
			loopSounds[entityNum].oldDopplerScale = loopSounds[entityNum].dopplerScale;
		}
		loopSounds[entityNum].dopplerScale = lenb / (lena * 100);
		if (loopSounds[entityNum].dopplerScale <= 1.0)
		{
			loopSounds[entityNum].doppler = false;			// don't bother doing the math
		}
	}

	loopSounds[entityNum].framenum = S_GetClientFrameCount();
}

//==========================================================================
//
//	S_AddRealLoopingSound
//
//	Called during entity generation for a frame
//	Include velocity in case I get around to doing doppler...
//
//==========================================================================

void S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfxHandle)
{
	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	if (sfxHandle < 0 || sfxHandle >= s_numSfx)
	{
		if (GGameType & GAME_Quake3)
		{
			GLog.Write(S_COLOR_YELLOW "S_AddRealLoopingSound: handle %i out of range\n", sfxHandle);
		}
		else
		{
			GLog.Write("S_AddRealLoopingSound: handle %i out of range\n", sfxHandle);
		}
		return;
	}

	sfx_t* sfx = &s_knownSfx[sfxHandle];

	if (sfx->InMemory == false)
	{
		S_LoadSound(sfx);
	}

	if (!sfx->Length)
	{
		QDropException(va("%s has length 0", sfx->Name));
	}

	VectorCopy(origin, loopSounds[entityNum].origin);
	VectorCopy(velocity, loopSounds[entityNum].velocity);
	loopSounds[entityNum].sfx = sfx;
	loopSounds[entityNum].active = true;
	loopSounds[entityNum].kill = false;
	loopSounds[entityNum].doppler = false;
}

//==========================================================================
//
//	S_PickChannel
//
//==========================================================================

channel_t* S_PickChannel(int EntNum, int EntChannel)
{
	if ((GGameType & GAME_Quake2) && EntChannel < 0)
	{
		QDropException("S_PickChannel: entchannel<0");
	}

	// Check for replacement sound, or find the best one to replace
	int FirstToDie = -1;
	int LifeLeft = 0x7fffffff;
	for (int ChIdx = 0; ChIdx < MAX_CHANNELS; ChIdx++)
    {
		if (EntChannel != 0 &&		// channel 0 never overrides
			s_channels[ChIdx].entnum == EntNum &&
			(s_channels[ChIdx].entchannel == EntChannel || EntChannel == -1))
		{
			// always override sound from same entity
			FirstToDie = ChIdx;
			break;
		}

		// don't let monster sounds override player sounds
		if (s_channels[ChIdx].entnum == listener_number && EntNum != listener_number && s_channels[ChIdx].sfx)
		{
			continue;
		}

		if (!s_channels[ChIdx].sfx)
		{
			if (LifeLeft > 0)
			{
				LifeLeft = 0;
				FirstToDie = ChIdx;
			}
			continue;
		}

		if (s_channels[ChIdx].startSample + s_channels[ChIdx].sfx->Length - s_paintedtime < LifeLeft)
		{
			LifeLeft = s_channels[ChIdx].startSample + s_channels[ChIdx].sfx->Length - s_paintedtime;
			FirstToDie = ChIdx;
		}
	}

	if (FirstToDie == -1)
	{
		return NULL;
	}

	channel_t* Ch = &s_channels[FirstToDie];
	Com_Memset(Ch, 0, sizeof(*Ch));

	return Ch;
}       

//==========================================================================
//
//	S_SpatializeOrigin
//
//	Used for spatializing channels.
//
//==========================================================================

void S_SpatializeOrigin(vec3_t origin, int master_vol, float dist_mult, int *left_vol, int *right_vol)
{
	vec_t		dot;
	vec_t		dist;
	vec_t		lscale, rscale, scale;
	vec3_t		source_vec;
	vec3_t		vec;

	// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	if (!(GGameType & GAME_QuakeHexen))
	{
		dist -= SOUND_FULLVOLUME;
		if (dist < 0)
		{
			dist = 0;			// close enough to be at full volume
		}
	}
	dist *= dist_mult;		// different attenuation levels

	VectorRotate( source_vec, listener_axis, vec );

	dot = -vec[1];

	if (dma.channels == 1 || ((GGameType & GAME_Quake2) && !dist_mult))
	{
		// no attenuation = no spatialization
		rscale = 1.0;
		lscale = 1.0;
	}
	else if (GGameType & GAME_QuakeHexen)
	{
		rscale = 1.0 + dot;
		lscale = 1.0 - dot;
	}
	else
	{
		rscale = 0.5 * (1.0 + dot);
		lscale = 0.5 * (1.0 - dot);
		if (rscale < 0)
		{
			rscale = 0;
		}
		if (lscale < 0)
		{
			lscale = 0;
		}
	}

	// add in distance effect
	scale = (1.0 - dist) * rscale;
	*right_vol = (int)(master_vol * scale);
	if (*right_vol < 0)
	{
		*right_vol = 0;
	}

	scale = (1.0 - dist) * lscale;
	*left_vol = (int)(master_vol * scale);
	if (*left_vol < 0)
	{
		*left_vol = 0;
	}
}

//==========================================================================
//
//	S_ClearSoundBuffer
//
//	If we are about to perform file access, clear the buffer
// so sound doesn't stutter.
//
//==========================================================================

void S_ClearSoundBuffer()
{
	int		clear;
		
	if (!s_soundStarted)
		return;

	if (GGameType & GAME_Quake3)
	{
		// stop looping sounds
		Com_Memset(loopSounds, 0, MAX_LOOPSOUNDS * sizeof(loopSound_t));
		Com_Memset(loop_channels, 0, MAX_CHANNELS * sizeof(channel_t));
		numLoopChannels = 0;

		S_ChannelSetup();
	}

	s_rawend = 0;

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	SNDDMA_BeginPainting();
	if (dma.buffer)
		// TTimo: due to a particular bug workaround in linux sound code,
		//   have to optionally use a custom C implementation of Com_Memset
		//   not affecting win32, we have #define Snd_Memset Com_Memset
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=371
		Snd_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit();
}

//==========================================================================
//
//	S_StopAllSounds
//
//==========================================================================

void S_StopAllSounds()
{
	if (!s_soundStarted)
	{
		return;
	}

	//	Clear all the playsounds.
	Com_Memset(s_playsounds, 0, sizeof(s_playsounds));
	s_freeplays.next = s_freeplays.prev = &s_freeplays;
	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

	for (int i = 0; i < MAX_PLAYSOUNDS; i++)
	{
		s_playsounds[i].prev = &s_freeplays;
		s_playsounds[i].next = s_freeplays.next;
		s_playsounds[i].prev->next = &s_playsounds[i];
		s_playsounds[i].next->prev = &s_playsounds[i];
	}

	if (!(GGameType & GAME_Quake3))
	{
		//	Clear all the channels.
		//	Quake 3 does this in S_ClearSoundBuffer.
		Com_Memset(s_channels, 0, sizeof(s_channels));
		Com_Memset(loop_channels, 0, sizeof(loop_channels));
	}

	if (GGameType & GAME_QuakeHexen)
	{
		numLoopChannels = BSP29_NUM_AMBIENTS;	// no statics
	}

	//	Stop the background music.
	S_StopBackgroundTrack();

	S_ClearSoundBuffer();
}

/*
=================
SND_Spatialize
=================
*/
void SND_Spatialize(channel_t *ch)
{
// anything coming from the view entity will allways be full volume
	if (ch->entnum == listener_number)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

	// calculate stereo seperation and distance attenuation
	S_SpatializeOrigin(ch->origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
}           
