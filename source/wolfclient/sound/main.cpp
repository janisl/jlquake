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
//**
//**	Main control for any streaming sound output device.
//**
//**************************************************************************
 
// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "../../client/sound/local.h"
#include "../../core/file_formats/bsp29.h"

// MACROS ------------------------------------------------------------------

#define MAX_PLAYSOUNDS			128

//	This is MAX_EDICTS or MAX_GENTITIES_Q3
#define MAX_LOOPSOUNDS			1024

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define SOUND_FULLVOLUME		80

#define SOUND_LOOPATTENUATE		0.003
#define SOUND_ATTENUATE			0.0008f
#define SOUND_RANGE_DEFAULT		1250

#define LOOP_HASH				128

//	some flags for queued music tracks
#define QUEUED_PLAY_ONCE		-1
#define QUEUED_PLAY_LOOPED		-2
#define QUEUED_PLAY_ONCE_SILENT	-3  // when done it goes quiet

#define UNDERWATER_BIT_WB		8
#define UNDERWATER_BIT_ET		16

// TYPES -------------------------------------------------------------------

struct loopSound_t
{
	vec3_t		origin;
	vec3_t		velocity;
	sfx_t		*sfx;
	int			mergeFrame;
	qboolean	active;
	qboolean	kill;
	qboolean	doppler;
	float		dopplerScale;
	float		oldDopplerScale;
	int			framenum;
	float range;            //----(SA)	added
	int vol;
	qboolean loudUnderWater;    // (SA) set if this sound should be played at full vol even when under water (under water loop sound for ex.)
	int startTime, startSample;         // ydnar: so looping sounds can be out of phase
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

sfx_t *S_RegisterSexedSound(int entnum, char *base);
int S_GetClFrameServertime();

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

//static 
void S_SpatializeOrigin(vec3_t origin, int master_vol, float dist_mult,
	int *left_vol, int *right_vol, float range, int noAttenuation);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

dma_t				dma;

int					s_soundtime;	// sample PAIRS
int   				s_paintedtime; 	// sample PAIRS

Cvar*				s_volume;
Cvar*				s_testsound;
Cvar*				s_khz;
Cvar*				s_bits;
Cvar*				s_channels_cv;
Cvar*				bgmvolume;
Cvar*				bgmtype;
Cvar* s_mute;	// for DM so he can 'toggle' sound on/off without disturbing volume levels

channel_t   		s_channels[MAX_CHANNELS];
channel_t   		loop_channels[MAX_CHANNELS];
int					numLoopChannels;

playsound_t			s_pendingplays;

// Streaming sounds
// !! NOTE: the first streaming sound is always the music
streamingSound_t streamingSounds[MAX_STREAMING_SOUNDS];
int						s_rawend[MAX_STREAMING_SOUNDS];
portable_samplepair_t	s_rawsamples[MAX_STREAMING_SOUNDS][MAX_RAW_SAMPLES];
// RF, store the volumes, since now they get adjusted at time of painting, so we can extract talking data first
portable_samplepair_t s_rawVolume[MAX_STREAMING_SOUNDS];

sfx_t				s_knownSfx[MAX_SFX];

float s_volCurrent;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

//static 
int			s_soundStarted;
//static 
bool			s_soundMuted;
//static
bool s_soundPainted;

bool			s_use_custom_memset = false;

//static 
Cvar*		s_show;
//static 
Cvar*		s_mixahead;
//static 
Cvar*		s_mixPreStep;
//static 
Cvar*		s_musicVolume;
//static 
Cvar*		s_doppler;
static Cvar*		s_ambient_level;
static Cvar*		s_ambient_fade;
static Cvar*		snd_noextraupdate;
Cvar* cl_cacheGathering;
Cvar* s_defaultsound;	// added to silence the default beep sound if desired
Cvar* s_debugMusic;

//static 
int			listener_number;
//static 
vec3_t		listener_origin;
//static 
vec3_t		listener_axis[3];

//static 
int			s_numSfx = 0;
//static 
sfx_t*		sfxHash[LOOP_HASH];

static channel_t* freelist = NULL;
static channel_t* endflist = NULL;

static sfx_t		*ambient_sfx[BSP29_NUM_AMBIENTS];

static int			s_registration_sequence;
static bool			s_registering;

static playsound_t	s_playsounds[MAX_PLAYSOUNDS];
static playsound_t	s_freeplays;

//static 
loopSound_t	loopSounds[MAX_LOOPSOUNDS];
//static 
int numLoopSounds;

static vec_t		sound_nominal_clip_dist=1000.0;

static int			s_beginofs;

char nextMusicTrack[MAX_QPATH];
int nextMusicTrackType;

int numStreamingSounds = 0;

vec3_t entityPositions[MAX_GENTITIES_Q3];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Snd_Memset
//
//==========================================================================

// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=371 
//static 
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

//static 
void S_ChannelFree(channel_t* v)
{
	v->sfx = NULL;
	v->threadReady = false;
	*(channel_t**)endflist = v;
	endflist = v;
	*(channel_t**)v = NULL;
}

//==========================================================================
//
//	S_ChannelMalloc
//
//==========================================================================

//static 
channel_t* S_ChannelMalloc()
{
	channel_t *v;
	if (freelist == NULL)
	{
		return NULL;
	}
	// RF, be careful not to lose our freelist
	if (*(channel_t**)freelist == NULL)
	{
		return NULL;
	}
	v = freelist;
	freelist = *(channel_t**)freelist;
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		v->allocTime = Sys_Milliseconds();
	}
	else
	{
		v->allocTime = Com_Milliseconds();
	}
	return v;
}

//==========================================================================
//
//	S_ChannelSetup
//
//==========================================================================

//static 
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

	endflist = q;
	*(channel_t**)q = NULL;
	freelist = p + MAX_CHANNELS - 1;
	Log::develWrite("Channel memory manager started\n");
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
		char Letter = String::ToLower(Name[i]);
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
		if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
		{
			Name = "*default*";
		}
		else
		{
			throw Exception("S_FindName: NULL\n");
		}
	}
	if (!Name[0])
	{
		if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
		{
			Name = "*default*";
		}
		else
		{
			throw Exception("S_FindName: empty name\n");
		}
	}

	if (String::Length(Name) >= MAX_QPATH)
	{
		throw Exception(va("Sound name too long: %s", Name));
	}

	// Ridah, caching
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET) &&  cl_cacheGathering->integer)
	{
		Cbuf_ExecuteText(EXEC_NOW, va("cache_usedfile sound %s\n", Name));
	}

	int Hash = S_HashSFXName(Name);

	sfx_t* Sfx = sfxHash[Hash];
	// see if already loaded
	while (Sfx)
	{
		if (!String::ICmp(Sfx->Name, Name))
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
			throw Exception("S_FindName: out of sfx_t");
		}
		s_numSfx++;
	}

	Sfx = &s_knownSfx[i];
	Com_Memset(Sfx, 0, sizeof(*Sfx));
	String::Cpy(Sfx->Name, Name);
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
	String::Cpy(S, TrueName);

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
			throw Exception("S_FindName: out of sfx_t");
		}
		s_numSfx++;
	}

	sfx_t* Sfx = &s_knownSfx[i];
	Com_Memset(Sfx, 0, sizeof(*Sfx));
	String::Cpy(Sfx->Name, AliasName);
	Sfx->RegistrationSequence = s_registration_sequence;
	Sfx->TrueName = S;

	int Hash = S_HashSFXName(AliasName);
	Sfx->HashNext = sfxHash[Hash];
	sfxHash[Hash] = Sfx;

	return Sfx;
}

static void S_DefaultSound(sfx_t* sfx)
{
	if (s_defaultsound->integer)
	{
		sfx->Length = 512;
	}
	else
	{
		sfx->Length = 8;
	}

	sfx->Data = new short[sfx->Length];

	if (s_defaultsound->integer)
	{
		for (int i = 0; i < sfx->Length; i++)
		{
			sfx->Data[i] = i;
		}
	}
	else
	{
		for (int i = 0 ; i < sfx->Length ; i++)
		{
			sfx->Data[i] = 0;
		}
	}
	sfx->LoopStart = -1;
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

	if (GGameType & GAME_Tech3)
	{
		s_soundMuted = false;		// we can play again

		if (s_numSfx == 0)
		{
			s_numSfx = 0;
			Com_Memset(s_knownSfx, 0, sizeof(s_knownSfx));
			Com_Memset(sfxHash, 0, sizeof(sfx_t*) * LOOP_HASH);

			if (GGameType & GAME_Quake3)
			{
				S_RegisterSound("sound/feedback/hit.wav");		// changed to a sound in baseq3
			}
			else
			{
				sfx_t* sfx = S_FindName("***DEFAULT***");
				S_DefaultSound(sfx);
			}
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

	if (String::Length(Name) >= MAX_QPATH)
	{
		Log::write("Sound name exceeds MAX_QPATH\n");
		return 0;
	}

	sfx_t* Sfx = S_FindName(Name);
	Sfx->RegistrationSequence = s_registration_sequence;
	if (Sfx->Data)
	{
		if ((GGameType & GAME_Tech3) && Sfx->DefaultSound)
		{
			Log::write(S_COLOR_YELLOW "WARNING: could not find %s - using default\n", Sfx->Name);
			return 0;
		}
		return Sfx - s_knownSfx;
	}

	Sfx->InMemory = false;

	if (!(GGameType & GAME_Quake2) || !s_registering)
		S_LoadSound(Sfx);

	if ((GGameType & GAME_Tech3) && Sfx->DefaultSound)
	{
		Log::write(S_COLOR_YELLOW "WARNING: could not find %s - using default\n", Sfx->Name);
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
//	STREAMING SOUND
//**************************************************************************

//==========================================================================
//
//	S_ByteSwapRawSamples
//
//	If raw data has been loaded in little endien binary form, this must be done.
//	If raw data was calculated, as with ADPCM, this should not be called.
//
//==========================================================================

void S_ByteSwapRawSamples(int samples, int width, int s_channels, byte *data)
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

void S_RawSamples(int samples, int rate, int width, int channels, const byte *data, float lvol, float rvol, int streamingIndex)
{
	int		i;
	int		src, dst;
	float	scale;
	int		intVolumeL;
	int		intVolumeR;

	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	// volume taken into account when mixed
	s_rawVolume[streamingIndex].left = 256 * lvol;
	s_rawVolume[streamingIndex].right = 256 * rvol;

	intVolumeL = 256;
	intVolumeR = 256;

	if (s_rawend[streamingIndex] < s_soundtime)
	{
		Log::develWrite("S_RawSamples: resetting minimum: %i < %i\n", s_rawend[streamingIndex], s_soundtime);
		s_rawend[streamingIndex] = s_soundtime;
	}

	scale = (float)rate / dma.speed;

//Com_Printf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend);
	if (channels == 2 && width == 2)
	{
		if (scale == 1.0)
		{	// optimized case
			for (i=0 ; i<samples ; i++)
			{
				dst = s_rawend[streamingIndex]&(MAX_RAW_SAMPLES-1);
				s_rawend[streamingIndex]++;
				s_rawsamples[streamingIndex][dst].left = ((short*)data)[i * 2] * intVolumeL;
				s_rawsamples[streamingIndex][dst].right = ((short*)data)[i * 2 + 1] * intVolumeR;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend[streamingIndex]&(MAX_RAW_SAMPLES-1);
				s_rawend[streamingIndex]++;
				s_rawsamples[streamingIndex][dst].left = ((short *)data)[src*2] * intVolumeL;
				s_rawsamples[streamingIndex][dst].right = ((short *)data)[src*2+1] * intVolumeR;
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
			dst = s_rawend[streamingIndex]&(MAX_RAW_SAMPLES-1);
			s_rawend[streamingIndex]++;
			s_rawsamples[streamingIndex][dst].left = ((short *)data)[src] * intVolumeL;
			s_rawsamples[streamingIndex][dst].right = ((short *)data)[src] * intVolumeR;
		}
	}
	else if (channels == 2 && width == 1)
	{
		intVolumeL *= 256;
		intVolumeR *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend[streamingIndex]&(MAX_RAW_SAMPLES-1);
			s_rawend[streamingIndex]++;
			s_rawsamples[streamingIndex][dst].left = ((char *)data)[src*2] * intVolumeL;
			s_rawsamples[streamingIndex][dst].right = ((char *)data)[src*2+1] * intVolumeR;
		}
	}
	else if (channels == 1 && width == 1)
	{
		intVolumeL *= 256;
		intVolumeR *= 256;

		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend[streamingIndex]&(MAX_RAW_SAMPLES-1);
			s_rawend[streamingIndex]++;
			s_rawsamples[streamingIndex][dst].left = (((byte *)data)[src]-128) * intVolumeL;
			s_rawsamples[streamingIndex][dst].right = (((byte *)data)[src]-128) * intVolumeR;
		}
	}

	if (s_rawend[streamingIndex] > s_soundtime + MAX_RAW_SAMPLES)
	{
		Log::develWrite("S_RawSamples: overflowed %i > %i\n", s_rawend[streamingIndex], s_soundtime);
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

	if (String::Cmp(Name, Chunk))
	{
		return 0;
	}

	return Len;
}

//  FIXME: record the starting cg.time of the sound, so we can determine the
//  position by looking at the current cg.time, this way pausing or loading a
//  savegame won't screw up the timing of important sounds
float S_StartStreamingSound(const char* intro, const char* loop, int entnum, int channel, int attenuation)
{
	int len;
	char dump[16];
	int i;
	streamingSound_t *ss;
	fileHandle_t fh;

	if (!s_soundStarted || s_soundMuted || cls.state != CA_ACTIVE)
	{
		return 0;
	}

	if (!intro || !intro[0])
	{
		if (loop && loop[0])
		{
			intro = loop;
		}
		else
		{
			intro = "";
		}
	}
	common->DPrintf("S_StartStreamingSound( %s, %s, %i, %i, %i )\n", intro, loop, entnum, channel, attenuation);

	// look for a free track, but first check for overriding a currently playing sound for this entity
	ss = NULL;
	if (entnum >= 0)
	{
		for (i = 1; i < MAX_STREAMING_SOUNDS; i++)
		{
			// track 0 is music/cinematics
			if (!streamingSounds[i].file)
			{
				continue;
			}
			// check to see if this character currently has another sound streaming on the same channel
			if (channel != Q3CHAN_AUTO && streamingSounds[i].entnum >= 0 &&
				streamingSounds[i].channel == channel && streamingSounds[i].entnum == entnum)
			{
				// found a match, override this channel
				streamingSounds[i].kill = true;
				ss = &streamingSounds[i];   // use this track to start the new stream
				break;
			}
		}
	}
	if (!ss)
	{
		// no need to override a current stream, so look for a free track
		for (i = 1; i < MAX_STREAMING_SOUNDS; i++)
		{
			// track 0 is music/cinematics
			if (!streamingSounds[i].file)
			{
				ss = &streamingSounds[i];
				break;
			}
		}
	}
	if (!ss)
	{
		if (!s_mute->integer)
		{
			// don't do the print if you're muted
			common->Printf("S_StartStreamingSound: No free streaming tracks\n");
		}
		return 0;
	}

	if (ss->loop && loop)
	{
		String::NCpyZ(ss->loop, loop, sizeof(ss->loop) - 4);
	}
	else
	{
		ss->loop[0] = 0;
	}

	String::NCpyZ(ss->name, intro, sizeof(ss->name) - 4);
	String::DefaultExtension(ss->name, sizeof(ss->name), ".wav");

	// close the current sound if present, but DON'T reset s_rawend
	if (ss->file)
	{
		FS_FCloseFile(ss->file);
		ss->file = 0;
	}

	if (!intro[0])
	{
		return 0;
	}

	//
	// open up a wav file and get all the info
	//
	FS_FOpenFileRead(ss->name, &fh, true);
	if (!fh)
	{
		common->Printf("Couldn't open streaming sound file %s\n", ss->name);
		return 0;
	}

	// skip the riff wav header

	FS_Read(dump, 12, fh);

	if (!S_FindWavChunk(fh, "fmt "))
	{
		common->Printf("No fmt chunk in %s\n", ss->name);
		FS_FCloseFile(fh);
		return 0;
	}

	// save name for soundinfo
	ss->info.format = FGetLittleShort(fh);
	ss->info.channels = FGetLittleShort(fh);
	ss->info.rate = FGetLittleLong(fh);
	FGetLittleLong(fh);
	FGetLittleShort(fh);
	ss->info.width = FGetLittleShort(fh) / 8;

	if (ss->info.format != WAV_FORMAT_PCM)
	{
		FS_FCloseFile(fh);
		common->Printf("Not a microsoft PCM format wav: %s\n", ss->name);
		return 0;
	}

	len = S_FindWavChunk(fh, "data");
	if (len == 0)
	{
		FS_FCloseFile(fh);
		common->Printf("No data chunk in %s\n", ss->name);
		return 0;
	}

	ss->info.samples = len / (ss->info.width * ss->info.channels);

	ss->samples = ss->info.samples;
	ss->channel = channel;
	ss->attenuation = attenuation;
	ss->entnum = entnum;
	ss->kill = false;

	ss->fadeStartVol = 0;
	ss->fadeStart = 0;
	ss->fadeEnd = 0;
	ss->fadeTargetVol = 0;

	ss->file = fh;
	numStreamingSounds++;

	return (ss->samples / (float)ss->info.rate) * 1000.f;
}

//==========================================================================
//
//	S_StartBackgroundTrack
//
//==========================================================================

void S_StartBackgroundTrack(const char* intro, const char* loop, int fadeupTime)
{
	int		len;
	char	dump[16];
	char loopMusic[MAX_QPATH];
	streamingSound_t* ss;
	fileHandle_t fh;

	// music is always track 0
	ss = &streamingSounds[0];

	if (fadeupTime < 0)
	{
		// queue, don't play until music fades to 0 or is stopped
		// -1 - queue to play once then return to music
		// -2 - queue to set as new looping music

		if (intro && String::Length(intro))
		{
			String::Cpy(nextMusicTrack, intro);
			nextMusicTrackType = fadeupTime;
			if (fadeupTime == -2)
			{
				Cvar_Set("s_currentMusic", intro); //----(SA)	so the savegame will have the right music
			}
			if (s_debugMusic->integer)
			{
				if (fadeupTime == -1)
				{
					common->Printf("MUSIC: StartBgTrack: queueing '%s' for play once\n", intro);
				}
				else if (fadeupTime == -2)
				{
					common->Printf("MUSIC: StartBgTrack: queueing '%s' as new loop\n", intro);
				}
			}
		}
		else
		{
			nextMusicTrack[0] = 0;  // clear out the next track so things go on as they are
			nextMusicTrackType = 0; // be quiet at the next opportunity

			// clear out looping sound in current music so that it'll stop when it's done
			if (ss && ss->loop)
			{
				ss->loop[0] = 0;    // clear loop
			}

			if (s_debugMusic->integer)
			{
				common->Printf("S_StartBgTrack(): queue cleared\n");
			}
		}

		return; // don't actually start any queued sounds
	}

	if (!s_soundStarted)
	{
		return;
	}

	if (!intro)
	{
		intro = "";
	}

	if (!loop || !loop[0])
	{
		String::NCpyZ(loopMusic, intro, sizeof(loopMusic));
	}
	else
	{
		String::NCpyZ(loopMusic, loop, sizeof(loopMusic));
	}
	if (!String::ICmp(loop, "onetimeonly"))
	{
		// don't change the loop if you're playing a single hit
		String::NCpyZ(loopMusic, ss->loop, sizeof(loopMusic));
	}

	Log::develWrite("S_StartBackgroundTrack( %s, %s )\n", intro, loopMusic);

	Cvar_Set("s_currentMusic", "");	//	so the savegame will have the right music

	String::NCpyZ(ss->name, intro, sizeof(ss->name) - 4);
	String::DefaultExtension(ss->name, sizeof(ss->name), ".wav");

	String::NCpyZ(ss->loop, loop, sizeof(ss->loop));

	// close the background track, but DON'T reset s_rawend
	// if restarting the same back ground track
	if (ss->file)
	{
		FS_FCloseFile(ss->file);
		ss->file = 0;
	}

	if (!intro[0])
	{
		return;
	}

	ss->channel = 0;
	ss->entnum = -1;
	ss->attenuation = 0;

	//
	// open up a wav file and get all the info
	//
	FS_FOpenFileRead(ss->name, &fh, true);
	if (!fh)
	{
		if (GGameType & GAME_Quake3)
			Log::write(S_COLOR_YELLOW "WARNING: couldn't open music file %s\n", ss->name);
		else
			Log::write("WARNING: couldn't open music file %s\n", ss->name);
		return;
	}

	// skip the riff wav header

	FS_Read(dump, 12, fh);

	if (!S_FindWavChunk(fh, "fmt "))
	{
		Log::write("No fmt chunk in %s\n", ss->name);
		FS_FCloseFile(fh);
		return;
	}

	// save name for soundinfo
	ss->info.format = FGetLittleShort(fh);
	ss->info.channels = FGetLittleShort(fh);
	ss->info.rate = FGetLittleLong(fh);
	FGetLittleLong(fh);
	FGetLittleShort(fh);
	ss->info.width = FGetLittleShort(fh) / 8;

	if (ss->info.format != WAV_FORMAT_PCM)
	{
		FS_FCloseFile(fh);
		Log::write("Not a microsoft PCM format wav: %s\n", ss->name);
		return;
	}

	if (ss->info.channels != 2 || ss->info.rate < 22050)
	{
		Log::write(S_COLOR_YELLOW "WARNING: music file %s is not 22k or higher stereo\n", ss->name);
	}

	if (( len = S_FindWavChunk(fh, "data")) == 0)
	{
		FS_FCloseFile(fh);
		Log::write("No data chunk in %s\n", ss->name);
		return;
	}

	if (s_debugMusic->integer)
	{
		common->Printf("MUSIC: StartBgTrack:\n   playing %s\n   looping %s %d\n", intro, loopMusic, fadeupTime);
	}

	Cvar_Set("s_currentMusic", loopMusic);	//	so the savegame will have the right music

	ss->info.samples = len / (ss->info.width * ss->info.channels);

	ss->samples = ss->info.samples;

	ss->fadeStartVol = 0;
	ss->fadeStart = 0;
	ss->fadeEnd = 0;
	ss->fadeTargetVol = 0;

	if (fadeupTime)
	{
		ss->fadeStart = s_soundtime;
		ss->fadeEnd = s_soundtime + (((float)(ss->info.rate ) / 1000.0f) * fadeupTime);
		ss->fadeTargetVol = 1.0;
	}

	ss->looped = 0;

	ss->file = fh;
	ss->kill = 0;
	numStreamingSounds++;

	common->DPrintf("S_StartBackgroundTrack - Success\n");
}

static void S_StopStreamingSound(int index)
{
	if (!streamingSounds[index].file)
	{
		return;
	}
	streamingSounds[index].kill = 1;
}

void S_StopEntStreamingSound(int entNum)
{
	if (GGameType & GAME_WolfSP && entNum < 0)
	{
		return;
	}

	for (int i = 1; i < MAX_STREAMING_SOUNDS; i++)
	{
		// track 0 is music/cinematics
		if (!streamingSounds[i].file)
		{
			continue;
		}

		// Gordon: -1 = ALL now
		if (streamingSounds[i].entnum != entNum && entNum != -1)
		{
			continue;
		}

		S_StopStreamingSound(i);
		s_rawend[i] = 0;    // stop it /now/
	}
}

//==========================================================================
//
//	S_StopBackgroundTrack
//
//==========================================================================

void S_StopBackgroundTrack()
{
	S_StopStreamingSound(0);
}

void S_FadeStreamingSound(float targetVol, int time, int ssNum)
{
	if (ssNum >= MAX_STREAMING_SOUNDS)
	{
		// invalid sound
		return;
	}

	streamingSound_t* ss = &streamingSounds[ssNum];

	if (!ss->file)
	{
		return;
	}

	if (ss->kill)
	{
		return;
	}

	ss->fadeStartVol = 1.0f;

	if (ssNum == 0)
	{
		if (s_debugMusic->integer)
		{
			common->Printf("MUSIC: Fade: %0.2f %d\n", targetVol, time);
		}
	}

	// get current fraction if already fading/faded
	if (ss->fadeStart)
	{
		if (ss->fadeEnd <= s_soundtime)
		{
			ss->fadeStartVol = ss->fadeTargetVol;
		}
		else
		{
			ss->fadeStartVol = ((float)(s_soundtime - ss->fadeStart) / (float)(ss->fadeEnd - ss->fadeStart));
		}
	}

	ss->fadeStart = s_soundtime;
	ss->fadeEnd = s_soundtime + (((float)ss->info.rate / 1000.0f) * time);
	ss->fadeTargetVol = targetVol;
}

static float S_GetStreamingFade(streamingSound_t* ss)
{
	float oldfrac, newfrac;

	if (!ss->fadeStart)
	{
		return 1.0f;    // full volume

	}
	if (ss->fadeEnd <= s_soundtime)
	{
		// it's hit it's target
		if (ss->fadeTargetVol <= 0)
		{
			// faded out.  die next update
			ss->kill = true;
		}
		return ss->fadeTargetVol;
	}

	newfrac = (float)(s_soundtime - ss->fadeStart) / (float)(ss->fadeEnd - ss->fadeStart);
	oldfrac = 1.0f - newfrac;

	return (oldfrac * ss->fadeStartVol) + (newfrac * ss->fadeTargetVol);
}

static int S_CheckForQueuedMusic()
{
	if (!nextMusicTrack[0])
	{
		// we didn't actually care about the length
		return 0;
	}

	char* nextMusicVA = va("%s", nextMusicTrack);

	streamingSound_t* ss = &streamingSounds[0];

	if (nextMusicTrackType == QUEUED_PLAY_ONCE_SILENT)
	{
		// do nothing.  current music is dead, don't start another
	}
	else if (nextMusicTrackType == QUEUED_PLAY_ONCE)
	{
		S_StartBackgroundTrack(nextMusicVA, ss->name, 0);      // play once, then go back to looping what's currently playing
	}
	else
	{        // QUEUED_PLAY_LOOPED
		S_StartBackgroundTrack(nextMusicVA, nextMusicVA, 0);   // take over
	}

	nextMusicTrackType = 0;     // clear out music queue
	if (!(GGameType & GAME_WolfSP))
	{
		nextMusicTrack[0] = 0;		// clear out music queue
	}

	return 1;
}

//==========================================================================
//
//	S_UpdateStreamingSounds
//
//==========================================================================

//static 
void S_UpdateStreamingSounds()
{
	int		bufferSamples;
	int		fileSamples;
	byte	raw[30000];		// just enough to fit in a mac stack frame
	int		fileBytes;
	int		r, i;
	streamingSound_t *ss;
	int     *re;
	float lvol, rvol;
	int soundMixAheadTime;
	float streamingVol = 1.0f;

	if (!s_soundStarted)
	{
		return;
	}

	soundMixAheadTime = s_soundtime; // + (int)(0.35 * dma.speed);	// allow for talking animations

	s_soundPainted = true;

	for (i = 0, ss = streamingSounds, re = s_rawend; i < MAX_STREAMING_SOUNDS; i++, ss++, re++)
	{
		if (ss->kill && ss->file)
		{
			FS_FCloseFile(ss->file);
			ss->file = 0;
			numStreamingSounds--;
			if (i == 0 || ss->kill == 2)	//  kill whole channel /now/
			{
				*re = 0;
			}
			ss->kill = 0;
			continue;
		}

		// don't bother playing anything if musicvolume is 0
		if (i == 0 && s_musicVolume->value <= 0)
		{
			continue;                       // skip until next frame
		}
		if (i > 0 && s_volume->value <= 0)
		{
			continue;
		}

		if (!ss->file)
		{
			if (i == 0)	// music
			{
				// quiet now, so start up queued music if it exists
				S_CheckForQueuedMusic();
			}
			continue;                       // skip until next frame
		}

		// see how many samples should be copied into the raw buffer
		if (*re < soundMixAheadTime)
		{
			*re = soundMixAheadTime;
		}

		while (*re < soundMixAheadTime + MAX_RAW_SAMPLES)
		{
			bufferSamples = MAX_RAW_SAMPLES - (*re - soundMixAheadTime);

			// decide how much data needs to be read from the file
			fileSamples = bufferSamples * ss->info.rate / dma.speed;

			// if there are no samples due to be read this frame, abort painting
			// but keep the streaming going, since it might just need to wait until
			// the next frame before it needs to paint some more
			if (!fileSamples)
			{
				//	Could happen if dma.speed > s_backgroundInfo.rate and
				// bufferSamples is 1
				break;
			}

			// don't try and read past the end of the file
			if (fileSamples > ss->samples)
			{
				fileSamples = ss->samples;
			}

			// our max buffer size
			fileBytes = fileSamples * (ss->info.width * ss->info.channels);
			if (fileBytes > (int)sizeof(raw))
			{
				fileBytes = sizeof(raw);
				fileSamples = fileBytes / (ss->info.width * ss->info.channels);
			}

			r = FS_Read(raw, fileBytes, ss->file);
			if (r != fileBytes)
			{
				Log::write("StreamedRead failure on stream sound\n");
				ss->kill = 1;
				break;
			}

			// byte swap if needed
			S_ByteSwapRawSamples(fileSamples, ss->info.width, ss->info.channels, raw);

			// calculate the volume
			streamingVol = S_GetStreamingFade(ss);

			streamingVol *= s_volCurrent; // get current global volume level

			if (s_mute->value)
			{
				//	sound is muted.  process to maintain timing, but play at 0 volume
				streamingVol = 0;
			}

			if (i == 0)   // music
			{
				lvol = rvol = s_musicVolume->value * streamingVol;
			}
			else
			{        // attenuate if required
				if (ss->entnum >= 0 && ss->attenuation)
				{
					int r, l;
					S_SpatializeOrigin(entityPositions[ss->entnum], s_volume->value * 255.0f, 1, &l, &r, SOUND_RANGE_DEFAULT, false);
					if ((lvol = ((float)l / 255.0)) > 1.0)
					{
						lvol = 1.0;
					}
					if ((rvol = ((float)r / 255.0)) > 1.0)
					{
						rvol = 1.0;
					}
					lvol *= streamingVol;
					rvol *= streamingVol;
				}
				else
				{
					lvol = rvol = s_volume->value * streamingVol;
				}
			}

			// add to raw buffer
			S_RawSamples(fileSamples, ss->info.rate, 
				ss->info.width, ss->info.channels, raw, lvol, rvol, i);

			ss->samples -= fileSamples;
			if (!ss->samples)
			{
				// Queued music will take over as the new loop
				// start up queued music if it exists
				if (i == 0 && nextMusicTrackType)
				{
					// queued music is queued
					if (ss->file)
					{
						FS_FCloseFile(ss->file);
						ss->file = 0;
						numStreamingSounds--;
						s_rawend[i] = 0;    // reset rawend
					}
					break;  // this is now the music ss->file, no need to re-start next time through
				}
				// loop
				else if (ss->loop[0])
				{
					if (ss->looped)
					{
						char dump[16];
						FS_Seek(ss->file, 0, FS_SEEK_SET);	// just go back to the beginning
						FS_Read(dump, 12, ss->file);

						if (!S_FindWavChunk(ss->file, "fmt "))
						{
							ss->kill = 1;
							break;
						}

						// save name for soundinfo
						ss->info.format = FGetLittleShort(ss->file);
						ss->info.channels = FGetLittleShort(ss->file);
						ss->info.rate = FGetLittleLong(ss->file);
						FGetLittleLong(ss->file);
						FGetLittleShort(ss->file);
						ss->info.width = FGetLittleShort(ss->file) / 8;
						ss->samples = ss->info.samples;
						if ((S_FindWavChunk(ss->file, "data")) == 0)
						{
							ss->kill = 1;
						}
						if (s_debugMusic->integer)
						{
							common->Printf("MUSIC: looping current track\n");
						}
						break;
					}
					else
					{                                            // start up the sound
						S_StartBackgroundTrack(ss->loop, ss->loop, 0);
						ss->looped = true;	// this is now the music ss->file, no need to re-start next time through
						break;
					}
				}
				else
				{
					// no loop, just stop
					ss->kill = 1;
					if (i == 0)
					{
						Cvar_Set("s_currentMusic", ""); //	so the savegame know's it's supposed to be quiet
						if (s_debugMusic->integer)
						{
							common->Printf("MUSIC: Ending current track-> no loop\n");
						}
					}
					break;
				}
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

//static 
void S_SoundInfo_f()
{
	Log::write("----- Sound Info -----\n");
	if (!s_soundStarted)
	{
		Log::write("sound system not started\n");
	}
	else
	{
		if (s_soundMuted)
		{
			Log::write("sound system is muted\n");
		}

		Log::write("%5d stereo\n", dma.channels - 1);
		Log::write("%5d samples\n", dma.samples);
		Log::write("%5d samplebits\n", dma.samplebits);
		Log::write("%5d submission_chunk\n", dma.submission_chunk);
		Log::write("%5d speed\n", dma.speed);
		Log::write("0x%p dma buffer\n", dma.buffer);
		if (streamingSounds[0].file)
		{
			Log::write("Background file: %s\n", streamingSounds[0].loop);
		}
		else
		{
			Log::write("No background file.\n" );
		}
	}
	Log::write("----------------------\n" );
}

//==========================================================================
//
//	S_Music_f
//
//==========================================================================

//static 
void S_Music_f()
{
	int c = Cmd_Argc();

	if (c == 2)
	{
		S_StartBackgroundTrack(Cmd_Argv(1), Cmd_Argv(1), 0);
		streamingSounds[0].loop[0] = 0;
	}
	else if (c == 3)
	{
		S_StartBackgroundTrack(Cmd_Argv(1), Cmd_Argv(2), 0);
	}
	else
	{
		Log::write("music <musicfile> [loopfile]\n");
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
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		if (EntityNum < 0 || EntityNum > MAX_GENTITIES_Q3)
		{
			common->Error("S_UpdateEntityPosition: bad entitynum %i", EntityNum);
		}
		VectorCopy(Origin, entityPositions[EntityNum]);
	}
	else
	{
		if (EntityNum < 0 || EntityNum > MAX_LOOPSOUNDS)
		{
			DropException(va("S_UpdateEntityPosition: bad entitynum %i", EntityNum));
		}
		VectorCopy(Origin, loopSounds[EntityNum].origin);
	}
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
	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		numLoopSounds = 0;
	}
	else if (GGameType & GAME_ET)
	{
		for (int i = 0; i < numLoopSounds; i++)
		{
			loopSounds[i].active = false;
		}
		numLoopSounds = 0;
		numLoopChannels = 0;
	}
	else
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
}

//==========================================================================
//
//	S_AddLoopingSound
//
//	Called during entity generation for a frame
//	Include velocity in case I get around to doing doppler...
//
//==========================================================================

void S_AddLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfxHandle, int volume, int soundTime)
{
	if (!s_soundStarted || s_soundMuted || cls.state != CA_ACTIVE)
	{
		return;
	}

	int index;
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		if (numLoopSounds >= MAX_LOOPSOUNDS)
		{
			return;
		}
		if (!volume)
		{
			return;
		}

		index = numLoopSounds;
		numLoopSounds++;
	}
	else
	{
		index = entityNum;
	}

	if (sfxHandle < 0 || sfxHandle >= s_numSfx)
	{
		if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
		{
			common->Error("S_AddLoopingSound: handle %i out of range", sfxHandle);
		}
		else if (GGameType & GAME_Quake3)
		{
			Log::write(S_COLOR_YELLOW "S_AddLoopingSound: handle %i out of range\n", sfxHandle);
		}
		else
		{
			Log::write("S_AddLoopingSound: handle %i out of range\n", sfxHandle);
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
		if (GGameType & GAME_Quake2)
		{
			return;
		}
		DropException(va("%s has length 0", sfx->Name));
	}

	VectorCopy(origin, loopSounds[index].origin);
	VectorCopy(velocity, loopSounds[index].velocity);
	loopSounds[index].active = true;
	loopSounds[index].kill = true;
	loopSounds[index].doppler = false;
	loopSounds[index].oldDopplerScale = 1.0;
	loopSounds[index].dopplerScale = 1.0;
	loopSounds[index].sfx = sfx;
	// ydnar: allow looped sounds to start when initially triggered, rather than in the middle of the sample
	loopSounds[index].startSample = soundTime % sfx->Length;

	if (range)
	{
		loopSounds[index].range = range;
	}
	else
	{
		loopSounds[index].range = SOUND_RANGE_DEFAULT;
	}

	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		if (volume & 1 << UNDERWATER_BIT_WB)
		{
			loopSounds[index].loudUnderWater = true;
		}

		if (volume > 255)
		{
			volume = 255;
		}
		else if (volume < 0)
		{
			volume = 0;
		}
	}
	else if (GGameType & GAME_ET)
	{
		if (volume & 1 << UNDERWATER_BIT_ET)
		{
			loopSounds[index].loudUnderWater = true;
		}

		if (volume > 65535)
		{
			volume = 65535;
		}
		else if (volume < 0)
		{
			volume = 0;
		}
	}

	if (GGameType & (GAME_WolfSP | GAME_ET))
	{
		loopSounds[index].vol = (int)( (float)volume * s_volCurrent);  //----(SA)	modified
	}
	else if (GGameType & GAME_WolfMP)
	{
		loopSounds[index].vol = volume;
	}

	if (s_doppler->integer && VectorLengthSquared(velocity) > 0.0)
	{
		vec3_t	out;
		float	lena, lenb;

		loopSounds[index].doppler = true;
		const float* listenerPos = GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET) ?
			entityPositions[listener_number] : loopSounds[listener_number].origin;
		lena = DistanceSquared(listenerPos, loopSounds[index].origin);
		VectorAdd(loopSounds[index].origin, loopSounds[index].velocity, out);
		lenb = DistanceSquared(listenerPos, out);
		if ((loopSounds[index].framenum + 1) != cls.framecount)
		{
			loopSounds[index].oldDopplerScale = 1.0;
		}
		else
		{
			loopSounds[index].oldDopplerScale = loopSounds[index].dopplerScale;
		}
		loopSounds[index].dopplerScale = lenb / (lena * 100);
		if (loopSounds[index].dopplerScale <= 1.0)
		{
			loopSounds[index].doppler = false;			// don't bother doing the math
		}
	}

	loopSounds[index].framenum = cls.framecount;
}

//==========================================================================
//
//	S_AddRealLoopingSound
//
//	Called during entity generation for a frame
//	Include velocity in case I get around to doing doppler...
//
//==========================================================================

void S_AddRealLoopingSound(int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfxHandle, int volume, int soundTime)
{
	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	int index;
	if (GGameType & GAME_ET)
	{
		if (numLoopSounds >= MAX_LOOPSOUNDS)
		{
			return;
		}

		if (!volume)
		{
			return;
		}
		index = numLoopSounds;
		numLoopSounds++;
	}
	else
	{
		index = entityNum;
	}

	if (sfxHandle < 0 || sfxHandle >= s_numSfx)
	{
		if (GGameType & GAME_Tech3)
		{
			Log::write(S_COLOR_YELLOW "S_AddRealLoopingSound: handle %i out of range\n", sfxHandle);
		}
		else
		{
			Log::write("S_AddRealLoopingSound: handle %i out of range\n", sfxHandle);
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
		DropException(va("%s has length 0", sfx->Name));
	}

	VectorCopy(origin, loopSounds[index].origin);
	VectorCopy(velocity, loopSounds[index].velocity);
	loopSounds[index].sfx = sfx;
	loopSounds[index].active = true;
	loopSounds[index].kill = false;
	loopSounds[index].doppler = false;

	if (GGameType & GAME_ET)
	{
		if (range)
		{
			loopSounds[index].range = range;
		}
		else
		{
			loopSounds[index].range = SOUND_RANGE_DEFAULT;
		}

		if (volume & 1 << UNDERWATER_BIT_ET)
		{
			loopSounds[index].loudUnderWater = true;
		}

		if (volume > 65535)
		{
			volume = 65535;
		}
		else if (volume < 0)
		{
			volume = 0;
		}

		loopSounds[index].vol = (int)((float)volume * s_volCurrent);  //----(SA)	modified
	}
}

//==========================================================================
//
//	S_PickChannel
//
//==========================================================================

static channel_t* S_PickChannel(int EntNum, int EntChannel)
{
	if ((GGameType & GAME_Quake2) && EntChannel < 0)
	{
		DropException("S_PickChannel: entchannel<0");
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

//static 
void S_SpatializeOrigin(vec3_t origin, int master_vol, float dist_mult,
	int *left_vol, int *right_vol, float range, int noAttenuation)
{
	vec_t		dot;
	vec_t		dist;
	vec_t		lscale, rscale, scale;
	vec3_t		source_vec;
	vec3_t		vec;
	float dist_fullvol;

	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		dist_fullvol = range * 0.064f;        // default range of 1250 gives 80
		dist_mult = dist_fullvol * 0.00001f;  // default range of 1250 gives .0008
	}
	else
	{
		dist_fullvol = SOUND_FULLVOLUME;
	}

	// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	if (!(GGameType & GAME_QuakeHexen))
	{
		dist -= dist_fullvol;
		if (dist < 0 || noAttenuation)
		{
			dist = 0;			// close enough to be at full volume
		}
	}
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		if (dist)
		{
			dist = dist / range;  // FIXME: lose the divide again
		}
	}
	else
	{
		dist *= dist_mult;		// different attenuation levels
	}

	VectorRotate( source_vec, listener_axis, vec );

	dot = -vec[1];

	if (dma.channels == 1 || noAttenuation || ((GGameType & GAME_Quake2) && !dist_mult))
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
	else if (GGameType & GAME_ET)
	{
		// xkan 11/22/2002 - the total energy of left + right should stay constant
		// and the energy is proportional to the square of sound volume.
		//
		// therefore lscale and rscale should satisfy the following 2 conditions:
		//		lscale^2 + rscale^2 = 2
		//		lscale^2 - rscale^2 = 2*vec[1]
		// (the second condition here is more experimental than physical).
		// These 2 conditions together give us the following solution.
		rscale = sqrt(1.0 - vec[1]);
		lscale = sqrt(1.0 + vec[1]);
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
//	S_Spatialize
//
//==========================================================================

static void S_Spatialize(channel_t *ch)
{
	vec3_t		origin;

	// anything coming from the view entity will always be full volume
	if (ch->entnum == listener_number)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

	if (ch->fixed_origin)
	{
		VectorCopy(ch->origin, origin);
	}
	else
	{
		VectorCopy(loopSounds[ch->entnum].origin, origin);
	}

	S_SpatializeOrigin(origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol, SOUND_RANGE_DEFAULT, false);
}           

void S_ClearSounds(bool clearStreaming, bool clearMusic)
{
	// stop looping sounds
	if (GGameType & GAME_ET)
	{
		Com_Memset(loopSounds, 0, MAX_GENTITIES_Q3 * sizeof(loopSound_t));
		Com_Memset(loop_channels, 0, MAX_CHANNELS * sizeof(channel_t));
		numLoopChannels = 0;
	}
	else
	{
		S_ClearLoopingSounds(true);
	}

	// RF, moved this up so streaming sounds dont get updated with the music, below,
	// and leave us with a snippet off streaming sounds after we reload
	// we don't want to stop guys with long dialogue from getting cut off by a file read
	if (clearStreaming)
	{
		// RF, clear talking amplitudes
		Com_Memset(s_entityTalkAmplitude, 0, sizeof(s_entityTalkAmplitude));

		streamingSound_t* ss = streamingSounds;
		for (int i = 0; i < MAX_STREAMING_SOUNDS; i++, ss++)
		{
			if (i > 0 || clearMusic)
			{
				s_rawend[i] = 0;
				ss->kill = 2;   // get rid of it next sound update
			}
		}

		// RF, we should also kill all channels, since we are killing streaming sounds
		// anyway (fixes siren in forest playing after a map_restart/loadgame
		channel_t* ch = s_channels;
		for (int i = 0; i < MAX_CHANNELS; i++, ch++)
		{
			if (ch->sfx)
			{
				S_ChannelFree(ch);
			}
		}

	}

	if (!clearMusic)
	{
		S_UpdateStreamingSounds();  //----(SA)	added so music will get updated if not cleared
	}
	else
	{
		// music cleanup
		nextMusicTrack[0] = 0;
		nextMusicTrackType = 0;
	}

	if (clearStreaming && clearMusic)
	{
		int clear;
		if (dma.samplebits == 8)
		{
			clear = 0x80;
		}
		else
		{
			clear = 0;
		}

		SNDDMA_BeginPainting();
		if (dma.buffer)
		{
			// TTimo: due to a particular bug workaround in linux sound code,
			//   have to optionally use a custom C implementation of Com_Memset
			//   not affecting win32, we have #define Snd_Memset Com_Memset
			// show_bug.cgi?id=371
			Snd_Memset(dma.buffer, clear, dma.samples * dma.samplebits / 8);
		}
		SNDDMA_Submit();

		if (GGameType & GAME_ET)
		{
			// NERVE - SMF - clear out channels so they don't finish playing when audio restarts
			S_ChannelSetup();
		}
	}
}

#if 0
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
#endif

//==========================================================================
//
//	S_AllocPlaysound
//
//==========================================================================

static playsound_t* S_AllocPlaysound()
{
	playsound_t	*ps;

	ps = s_freeplays.next;
	if (ps == &s_freeplays)
	{
		return NULL;		// no free playsounds
	}

	// unlink from freelist
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;
	
	return ps;
}

//==========================================================================
//
//	S_FreePlaysound
//
//==========================================================================

static void S_FreePlaysound(playsound_t* ps)
{
	// unlink from channel
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	// add to free list
	ps->next = s_freeplays.next;
	s_freeplays.next->prev = ps;
	ps->prev = &s_freeplays;
	s_freeplays.next = ps;
}

void S_ThreadStartSoundEx(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle,
	int flags, int volume)
{
	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	if (!origin && (entityNum < 0 || entityNum > MAX_GENTITIES_Q3))
	{
		common->Error("S_StartSound: bad entitynum %i", entityNum);
	}

	if (sfxHandle < 0 || sfxHandle >= s_numSfx)
	{
		common->Printf(S_COLOR_YELLOW "S_StartSound: handle %i out of range\n", sfxHandle);
		return;
	}

	sfx_t* sfx = &s_knownSfx[sfxHandle];

	if (s_show->integer == 1)
	{
		common->Printf("%i : %s\n", s_paintedtime, sfx->Name);
	}

	sfx->LastTimeUsed = Sys_Milliseconds();

	// check for a streaming sound that this entity is playing in this channel
	// kill it if it exists
	if (entityNum >= 0)
	{
		for (int i = 1; i < MAX_STREAMING_SOUNDS; i++)	// track 0 is music/cinematics
		{
			if (!streamingSounds[i].file)
			{
				continue;
			}
			// check to see if this character currently has another sound streaming on the same channel
			if ((entchannel != Q3CHAN_AUTO) && (streamingSounds[i].entnum >= 0) &&
				(streamingSounds[i].channel == entchannel) && (streamingSounds[i].entnum == entityNum))
			{
				// found a match, override this channel
				streamingSounds[i].kill = 1;
				break;
			}
		}
	}

	// shut off other sounds on this channel if necessary
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (s_channels[i].entnum == entityNum && s_channels[i].sfx && s_channels[i].entchannel == entchannel)
		{
			// cutoff all on channel
			if (flags & SND_CUTOFF_ALL)
			{
				S_ChannelFree(&s_channels[i]);
				continue;
			}

			if (s_channels[i].flags & SND_NOCUT)
			{
				continue;
			}

			// RF, let client voice sounds be overwritten
			if ((GGameType & GAME_WolfSP) && entityNum < MAX_CLIENTS_WS &&
				s_channels[i].entchannel != Q3CHAN_AUTO && s_channels[i].entchannel != Q3CHAN_WEAPON)
			{
				S_ChannelFree(&s_channels[i]);
				continue;
			}

			// cutoff sounds that expect to be overwritten
			if (s_channels[i].flags & SND_OKTOCUT)
			{
				S_ChannelFree(&s_channels[i]);
				continue;
			}

			// cutoff 'weak' sounds on channel
			if (flags & SND_CUTOFF)
			{
				if (s_channels[i].flags & SND_REQUESTCUT)
				{
					S_ChannelFree(&s_channels[i]);
					continue;
				}
			}
		}
	}

	channel_t* ch = NULL;


	// re-use channel if applicable
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (s_channels[i].entnum == entityNum && s_channels[i].entchannel == entchannel && entchannel != Q3CHAN_AUTO)
		{
			if (!(s_channels[i].flags & SND_NOCUT) && s_channels[i].sfx == sfx)
			{
				ch = &s_channels[i];
				break;
			}
		}
	}

	if (!ch)
	{
		ch = S_ChannelMalloc();
	}

	if (!ch)
	{
		ch = s_channels;

		int chosen = -1;
		int oldest = sfx->LastTimeUsed;
		for (int i = 0; i < MAX_CHANNELS; i++, ch++)
		{
			if (ch->entnum == entityNum && ch->sfx == sfx)
			{
				chosen = i;
				break;
			}
			if (ch->entnum != listener_number && ch->entnum == entityNum &&
				ch->allocTime < oldest && ch->entchannel != Q3CHAN_ANNOUNCER)
			{
				oldest = ch->allocTime;
				chosen = i;
			}
		}
		if (chosen == -1)
		{
			ch = s_channels;
			for (int i = 0; i < MAX_CHANNELS; i++, ch++)
			{
				if (ch->entnum != listener_number && ch->allocTime < oldest && ch->entchannel != Q3CHAN_ANNOUNCER)
				{
					oldest = ch->allocTime;
					chosen = i;
				}
			}
			if (chosen == -1)
			{
				if (ch->entnum == listener_number)
				{
					for (int i = 0; i < MAX_CHANNELS; i++, ch++)
					{
						if (ch->allocTime < oldest)
						{
							oldest = ch->allocTime;
							chosen = i;
						}
					}
				}
				if (chosen == -1)
				{
					//Com_Printf("dropping sound\n");
					return;
				}
			}
		}
		ch = &s_channels[chosen];
		ch->allocTime = sfx->LastTimeUsed;
	}

	if ( origin ) {
		VectorCopy( origin, ch->origin );
		ch->fixed_origin = true;
	} else {
		ch->fixed_origin = false;
	}

	ch->flags = flags;
	ch->master_vol = volume;
	ch->entnum = entityNum;
	ch->sfx = sfx;
	ch->entchannel = entchannel;
	ch->leftvol = ch->master_vol;       // these will get calced at next spatialize
	ch->rightvol = ch->master_vol;      // unless the game isn't running
	ch->doppler = false;

	if (ch->fixed_origin)
	{
		S_SpatializeOrigin(ch->origin, ch->master_vol, ch->dist_mult,
			&ch->leftvol, &ch->rightvol, SOUND_RANGE_DEFAULT, flags & SND_NO_ATTENUATION);
	}
	else
	{
		S_SpatializeOrigin(entityPositions[ch->entnum], ch->master_vol, ch->dist_mult,
			&ch->leftvol, &ch->rightvol, SOUND_RANGE_DEFAULT, flags & SND_NO_ATTENUATION);
	}

	ch->startSample = START_SAMPLE_IMMEDIATE;
	ch->threadReady = true;
}

void S_StartSoundEx(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle, int flags, int volume)
{
	if (!s_soundStarted || s_soundMuted || (cls.state != CA_ACTIVE && cls.state != CA_DISCONNECTED))
	{
		return;
	}

	// RF, we have lots of NULL sounds using up valuable channels, so just ignore them
	if ((GGameType & GAME_WolfSP) && !sfxHandle && entchannel != Q3CHAN_WEAPON)
	{
		// let null weapon sounds try to play.  they kill any weapon sounds playing when a guy dies
		return;
	}

	// RF, make the call now, or else we could override following streaming sounds in the same frame, due to the delay
	S_ThreadStartSoundEx(origin, entityNum, entchannel, sfxHandle, flags, volume);
}

//==========================================================================
//
//	S_StartSound
//
//	Validates the parms and ques the sound up
//	if pos is NULL, the sound will be dynamically sourced from the entity
//	Entchannel 0 will never override a playing sound
//
//==========================================================================

void S_StartSound(const vec3_t origin, int entnum, int entchannel, sfxHandle_t sfxHandle, float fvol, float attenuation, float timeofs)
{
	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		S_StartSoundEx(origin, entnum, entchannel, sfxHandle, 0, fvol * 255);
		return;
	}

	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	if (sfxHandle < 0 || sfxHandle >= s_numSfx)
	{
		if (GGameType & GAME_Quake3)
		{
			Log::write(S_COLOR_YELLOW "S_StartSound: handle %i out of range\n", sfxHandle);
		}
		else
		{
			Log::write("S_StartSound: handle %i out of range\n", sfxHandle);
		}
		return;
	}

	sfx_t* sfx = &s_knownSfx[sfxHandle];

	if (GGameType & GAME_QuakeHexen)
	{
		channel_t *target_chan, *check;
		int		vol;
		int		ch_idx;
		int		skip;
		qboolean skip_dist_check = false;

		vol = fvol * 255;

		// pick a channel to play on
		target_chan = S_PickChannel(entnum, entchannel);
		if (!target_chan)
			return;
			
		if ((GGameType & GAME_Hexen2) && attenuation == 4)//Looping sound- always play
		{
			skip_dist_check = true;
			attenuation = 1;//was 3 - static
		}

		// spatialize
		Com_Memset(target_chan, 0, sizeof(*target_chan));
		VectorCopy(origin, target_chan->origin);
		target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
		target_chan->master_vol = vol;
		target_chan->entnum = entnum;
		target_chan->entchannel = entchannel;
		target_chan->fixed_origin = true;
		S_Spatialize(target_chan);

		if (!skip_dist_check)
		{
			if (!target_chan->leftvol && !target_chan->rightvol)
			{
				return;		// not audible at all
			}
		}

		// new channel
		if (!S_LoadSound(sfx))
		{
			target_chan->sfx = NULL;
			return;		// couldn't load the sound's data
		}

		target_chan->sfx = sfx;
		target_chan->startSample = s_paintedtime;

		// if an identical sound has also been started this frame, offset the pos
		// a bit to keep it from just making the first one louder
		check = s_channels;
		for (ch_idx=0; ch_idx < MAX_CHANNELS ; ch_idx++, check++)
		{
			if (check == target_chan)
				continue;
			if (check->sfx == sfx && check->startSample == s_paintedtime)
			{
				skip = rand () % (int)(0.1*dma.speed);
				if (skip >= target_chan->startSample + target_chan->sfx->Length)
					skip = target_chan->startSample + target_chan->sfx->Length - 1;
				target_chan->startSample -= skip;
				break;
			}
			
		}
	}
	else if (GGameType & GAME_Quake2)
	{
		int			vol;
		playsound_t	*ps, *sort;
		int			start;

		if (sfx->Name[0] == '*')
		{
			sfx = S_RegisterSexedSound(entnum, sfx->Name);
		}

		// make sure the sound is loaded
		if (!S_LoadSound (sfx))
			return;		// couldn't load the sound's data

		vol = fvol*255;

		// make the playsound_t
		ps = S_AllocPlaysound ();
		if (!ps)
			return;

		if (origin)
		{
			VectorCopy (origin, ps->origin);
			ps->fixed_origin = true;
		}
		else
			ps->fixed_origin = false;

		ps->entnum = entnum;
		ps->entchannel = entchannel;
		ps->attenuation = attenuation;
		ps->volume = vol;
		ps->sfx = sfx;

		// drift s_beginofs
		start = S_GetClFrameServertime() * 0.001 * dma.speed + s_beginofs;
		if (start < s_paintedtime)
		{
			start = s_paintedtime;
			s_beginofs = start - (S_GetClFrameServertime() * 0.001 * dma.speed);
		}
		else if (start > s_paintedtime + 0.3 * dma.speed)
		{
			start = s_paintedtime + 0.1 * dma.speed;
			s_beginofs = start - (S_GetClFrameServertime() * 0.001 * dma.speed);
		}
		else
		{
			s_beginofs-=10;
		}

		if (!timeofs)
			ps->begin = s_paintedtime;
		else
			ps->begin = start + timeofs * dma.speed;

		// sort into the pending sound list
		for (sort = s_pendingplays.next ; 
			sort != &s_pendingplays && sort->begin < ps->begin ;
			sort = sort->next)
				;

		ps->next = sort;
		ps->prev = sort->prev;

		ps->next->prev = ps;
		ps->prev->next = ps;
	}
	else if (GGameType & GAME_Quake3)
	{
		channel_t	*ch;
		int i, oldest, chosen, time;
		int	inplay, allowed;

		if (!origin && ( entnum < 0 || entnum > MAX_LOOPSOUNDS))
		{
			throw DropException(va("S_StartSound: bad entitynum %i", entnum));
		}

		if (sfx->InMemory == false)
		{
			S_LoadSound(sfx);
		}

		if (s_show->integer == 1)
		{
			Log::write("%i : %s\n", s_paintedtime, sfx->Name );
		}

		time = Com_Milliseconds();

	//	Com_Printf("playing %s\n", sfx->soundName);
		// pick a channel to play on

		allowed = 4;
		if (entnum == listener_number)
		{
			allowed = 8;
		}

		ch = s_channels;
		inplay = 0;
		for (i = 0; i < MAX_CHANNELS ; i++, ch++ )
		{
			if (ch[i].entnum == entnum && ch[i].sfx == sfx)
			{
				if (time - ch[i].allocTime < 50)
				{
					return;
				}
				inplay++;
			}
		}

		if (inplay > allowed)
		{
			return;
		}

		sfx->LastTimeUsed = time;

		ch = S_ChannelMalloc();	// entityNum, entchannel);
		if (!ch)
		{
			ch = s_channels;

			oldest = sfx->LastTimeUsed;
			chosen = -1;
			for (i = 0 ; i < MAX_CHANNELS; i++, ch++)
			{
				if (ch->entnum != listener_number && ch->entnum == entnum && ch->allocTime<oldest && ch->entchannel != Q3CHAN_ANNOUNCER)
				{
					oldest = ch->allocTime;
					chosen = i;
				}
			}
			if (chosen == -1)
			{
				ch = s_channels;
				for (i = 0 ; i < MAX_CHANNELS; i++, ch++)
				{
					if (ch->entnum != listener_number && ch->allocTime<oldest && ch->entchannel != Q3CHAN_ANNOUNCER)
					{
						oldest = ch->allocTime;
						chosen = i;
					}
				}
				if (chosen == -1)
				{
					if (ch->entnum == listener_number)
					{
						for (i = 0 ; i < MAX_CHANNELS ; i++, ch++)
						{
							if (ch->allocTime < oldest)
							{
								oldest = ch->allocTime;
								chosen = i;
							}
						}
					}
					if (chosen == -1)
					{
						Log::write("dropping sound\n");
						return;
					}
				}
			}
			ch = &s_channels[chosen];
			ch->allocTime = sfx->LastTimeUsed;
		}

		if (origin)
		{
			VectorCopy (origin, ch->origin);
			ch->fixed_origin = true;
		}
		else
		{
			ch->fixed_origin = false;
		}

		ch->master_vol = 127;
		ch->entnum = entnum;
		ch->sfx = sfx;
		ch->startSample = START_SAMPLE_IMMEDIATE;
		ch->entchannel = entchannel;
		ch->leftvol = ch->master_vol;		// these will get calced at next spatialize
		ch->rightvol = ch->master_vol;		// unless the game isn't running
		ch->doppler = false;
		ch->dist_mult = SOUND_ATTENUATE;
	}
}

//==========================================================================
//
//	S_StartLocalSound
//
//==========================================================================

void S_StartLocalSound(const char* Sound)
{
	if (!s_soundStarted)
	{
		return;
	}

	sfxHandle_t sfx = S_RegisterSound(Sound);

	if (GGameType & GAME_QuakeHexen)
	{
		S_StartSound(vec3_origin, listener_number, -1, sfx);
	}
	else
	{
		S_StartSound(NULL, listener_number, 0, sfx);
	}
}

//==========================================================================
//
//	S_StartLocalSound
//
//==========================================================================

void S_StartLocalSound(sfxHandle_t sfxHandle, int channelNumber, int volume)
{
	S_StartSound(NULL, listener_number, channelNumber, sfxHandle, volume / 255.0);
}

//==========================================================================
//
//	S_IssuePlaysound
//
//	Take the next playsound and begin it on the channel
// This is never called directly by S_Play*, but only by the update loop.
//
//==========================================================================

void S_IssuePlaysound(playsound_t* ps)
{
	channel_t	*ch;

	if (s_show->value)
		Log::write("Issue %i\n", ps->begin);
	// pick a channel to play on
	ch = S_PickChannel(ps->entnum, ps->entchannel);
	if (!ch)
	{
		S_FreePlaysound(ps);
		return;
	}

	// spatialize
	if (ps->attenuation == ATTN_STATIC)
		ch->dist_mult = ps->attenuation * 0.001;
	else
		ch->dist_mult = ps->attenuation * 0.0005;
	ch->master_vol = ps->volume;
	ch->entnum = ps->entnum;
	ch->entchannel = ps->entchannel;
	ch->sfx = ps->sfx;
	VectorCopy (ps->origin, ch->origin);
	ch->fixed_origin = ps->fixed_origin;

	S_Spatialize(ch);

	S_LoadSound(ch->sfx);
    ch->startSample = s_paintedtime;

	// free the playsound
	S_FreePlaysound(ps);
}

//==========================================================================
//
//	S_StopSound
//
//==========================================================================

void S_StopSound(int entnum, int entchannel)
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (s_channels[i].entnum == entnum &&
			(((GGameType & GAME_Hexen2) && !entchannel) || s_channels[i].entchannel == entchannel))	// 0 matches any
		{
			s_channels[i].startSample = 0;
			s_channels[i].sfx = NULL;
			if (!(GGameType & GAME_Hexen2) || entchannel)
			{
				return;	//got a match, not looking for more.
			}
		}
	}
}

//==========================================================================
//
//	S_UpdateSoundPos
//
//==========================================================================

void S_UpdateSoundPos(int entnum, int entchannel, vec3_t origin)
{
	for (int i = 0; i < MAX_CHANNELS; i++)
	{
		if (s_channels[i].entnum == entnum &&
			s_channels[i].entchannel == entchannel)
		{
			VectorCopy(origin, s_channels[i].origin);
			return;
		}
	}
}

//==========================================================================
//
//	S_StaticSound
//
//==========================================================================

void S_StaticSound(sfxHandle_t Handle, vec3_t origin, float vol, float attenuation)
{
	if (Handle < 0 || Handle >= s_numSfx)
	{
		return;
	}
	sfx_t* sfx = s_knownSfx + Handle;

	if (numLoopChannels == MAX_CHANNELS)
	{
		Log::write("StaticSound: MAX_CHANNELS reached\n");
		Log::write(" failed at (%.2f, %.2f, %.2f)\n",origin[0],origin[1],origin[2]);
		return;
	}

	channel_t* ss = &loop_channels[numLoopChannels];
	numLoopChannels++;

	if (!S_LoadSound(sfx))
	{
		return;
	}

	if (sfx->LoopStart == -1)
	{
		Log::write("Sound %s not looped\n", sfx->Name);
		return;
	}

	ss->sfx = sfx;
	VectorCopy(origin, ss->origin);
	ss->master_vol = vol;
	ss->dist_mult = (attenuation / 64) / sound_nominal_clip_dist;
    ss->startSample = s_paintedtime;
	ss->fixed_origin = true;
}

#if 0
//==========================================================================
//
//	S_ScanChannelStarts
//
//	Returns qtrue if any new sounds were started since the last mix
//
//==========================================================================

static bool S_ScanChannelStarts()
{
	bool newSamples = false;
	channel_t* ch = s_channels;

	for (int i = 0; i < MAX_CHANNELS; i++, ch++)
	{
		if (!ch->sfx)
		{
			continue;
		}
		// if this channel was just started this frame,
		// set the sample count to it begins mixing
		// into the very first sample
		if (ch->startSample == START_SAMPLE_IMMEDIATE)
		{
			ch->startSample = s_paintedtime;
			newSamples = true;
			continue;
		}

		// if it is completely finished by now, clear it
		if (ch->startSample + (ch->sfx->Length) <= s_paintedtime)
		{
			S_ChannelFree(ch);
		}
	}

	return newSamples;
}

//==========================================================================
//
//	S_UpdateAmbientSounds
//
//==========================================================================

static void S_UpdateAmbientSounds()
{
	float		vol;

	// calc ambient sound levels
	const byte* ambient_sound_level = CM_LeafAmbientSoundLevel(CM_PointLeafnum(listener_origin));
	if (!ambient_sound_level || !s_ambient_level->value)
	{
		for (int ambient_channel = 0 ; ambient_channel< BSP29_NUM_AMBIENTS ; ambient_channel++)
		{
			loop_channels[ambient_channel].sfx = NULL;
		}
		return;
	}

	for (int ambient_channel = 0 ; ambient_channel< BSP29_NUM_AMBIENTS ; ambient_channel++)
	{
		channel_t* chan = &loop_channels[ambient_channel];	
		chan->sfx = ambient_sfx[ambient_channel];
	
		vol = s_ambient_level->value * ambient_sound_level[ambient_channel];
		if (vol < 8)
		{
			vol = 0;
		}

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += ((float)cls.frametime / 1000.0) * s_ambient_fade->value;
			if (chan->master_vol > vol)
			{
				chan->master_vol = vol;
			}
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= ((float)cls.frametime / 1000.0) * s_ambient_fade->value;
			if (chan->master_vol < vol)
			{
				chan->master_vol = vol;
			}
		}

		chan->leftvol = chan->rightvol = chan->master_vol;
	}

	channel_t* combine = NULL;

// update spatialization for static sounds
	channel_t* ch = &loop_channels[BSP29_NUM_AMBIENTS];
	for (int i = BSP29_NUM_AMBIENTS; i < numLoopChannels; i++, ch++)
	{
		if (!ch->sfx)
		{
			continue;
		}
		S_SpatializeOrigin(ch->origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
		if (!ch->leftvol && !ch->rightvol)
		{
			continue;
		}

		// try to combine static sounds with a previous channel of the same
		// sound effect so we don't mix five torches every frame
	
		// see if it can just use the last one
		if (combine && combine->sfx == ch->sfx)
		{
			combine->leftvol += ch->leftvol;
			combine->rightvol += ch->rightvol;
			ch->leftvol = ch->rightvol = 0;
			continue;
		}
		// search for one
		combine = &loop_channels[BSP29_NUM_AMBIENTS];
		int j;
		for (j = BSP29_NUM_AMBIENTS; j < i; j++, combine++)
		{
			if (combine->sfx == ch->sfx)
			{
				break;
			}
		}

		if (j == numLoopChannels)
		{
			combine = NULL;
		}
		else
		{
			if (combine != ch)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
			}
			continue;
		}
	}
}

//==========================================================================
//
//	S_AddLoopSounds
//
//	Spatialize all of the looping sounds. All sounds are on the same cycle,
// so any duplicates can just sum up the channel multipliers.
//
//==========================================================================

static void S_AddLoopSounds()
{
	int			left_total, right_total, left, right;
	static int	loopFrame;

	numLoopChannels = 0;

	int time = Com_Milliseconds();

	loopFrame++;
	for (int i = 0; i < MAX_LOOPSOUNDS; i++)
	{
		loopSound_t* loop = &loopSounds[i];
		if (!loop->active || loop->mergeFrame == loopFrame)
		{
			continue;	// already merged into an earlier sound
		}

		if (GGameType & GAME_Quake2)
		{
			S_SpatializeOrigin(loop->origin, 255.0, SOUND_LOOPATTENUATE, &left_total, &right_total);
		}
		else if (loop->kill)
		{
			S_SpatializeOrigin(loop->origin, 127, SOUND_ATTENUATE, &left_total, &right_total);			// 3d
		}
		else
		{
			S_SpatializeOrigin(loop->origin, 90,  SOUND_ATTENUATE, &left_total, &right_total);			// sphere
		}

		loop->sfx->LastTimeUsed = time;

		for (int j = i + 1; j < MAX_LOOPSOUNDS; j++)
		{
			loopSound_t* loop2 = &loopSounds[j];
			if (!loop2->active || loop2->doppler || loop2->sfx != loop->sfx)
			{
				continue;
			}
			loop2->mergeFrame = loopFrame;

			if (GGameType & GAME_Quake2)
			{
				S_SpatializeOrigin(loop2->origin, 255.0, SOUND_LOOPATTENUATE, &left, &right);
			}
			else if (loop2->kill)
			{
				S_SpatializeOrigin(loop2->origin, 127, SOUND_ATTENUATE, &left, &right);				// 3d
			}
			else
			{
				S_SpatializeOrigin(loop2->origin, 90,  SOUND_ATTENUATE, &left, &right);				// sphere
			}

			loop2->sfx->LastTimeUsed = time;
			left_total += left;
			right_total += right;
		}

		if (left_total == 0 && right_total == 0)
		{
			continue;		// not audible
		}

		// allocate a channel
		channel_t* ch = &loop_channels[numLoopChannels];

		if (left_total > 255)
		{
			left_total = 255;
		}
		if (right_total > 255)
		{
			right_total = 255;
		}

		ch->master_vol = 127;
		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->sfx = loop->sfx;
		ch->doppler = loop->doppler;
		ch->dopplerScale = loop->dopplerScale;
		ch->oldDopplerScale = loop->oldDopplerScale;
		numLoopChannels++;
		if (numLoopChannels == MAX_CHANNELS)
		{
			return;
		}
	}
}

//==========================================================================
//
//	S_Respatialize
//
//	Change the volumes of all the playing sounds for changes in their positions
//
//==========================================================================

void S_Respatialize(int entityNum, const vec3_t head, vec3_t axis[3], int inwater)
{
	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	listener_number = entityNum;
	VectorCopy(head, listener_origin);
	VectorCopy(axis[0], listener_axis[0]);
	VectorCopy(axis[1], listener_axis[1]);
	VectorCopy(axis[2], listener_axis[2]);

	// update spatialization for dynamic sounds	
	channel_t* ch = s_channels;
	for (int i = 0; i < MAX_CHANNELS; i++, ch++)
	{
		if (!ch->sfx)
		{
			continue;
		}
		// anything coming from the view entity will always be full volume
		if (ch->entnum == listener_number)
		{
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		}
		else
		{
			vec3_t origin;
			if (ch->fixed_origin)
			{
				VectorCopy(ch->origin, origin);
			}
			else
			{
				VectorCopy(loopSounds[ch->entnum].origin, origin);
			}

			S_SpatializeOrigin(origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
		}
		if ((GGameType & GAME_Quake2) && !ch->leftvol && !ch->rightvol)
		{
			Com_Memset(ch, 0, sizeof(*ch));
			continue;
		}
	}

	if (GGameType & GAME_QuakeHexen)
	{
		// update general area ambient sound sources
		S_UpdateAmbientSounds();
	}
	else
	{
		// add loopsounds
		S_AddLoopSounds();
	}
}

//==========================================================================
//
//	GetSoundtime
//
//==========================================================================

static void GetSoundtime()
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;
	
	fullsamples = dma.samples / dma.channels;

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (s_paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_StopAllSounds();
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers*fullsamples + samplepos/dma.channels;

	if (GGameType & GAME_Quake3)
	{
#if 0
		// check to make sure that we haven't overshot
		if (s_paintedtime < s_soundtime)
		{
			Com_DPrintf ("S_Update_ : overflow\n");
			s_paintedtime = s_soundtime;
		}
#endif

#ifdef _WIN32
		if (dma.submission_chunk < 256)
		{
			s_paintedtime = s_soundtime + s_mixPreStep->value * dma.speed;
		}
		else
		{
			s_paintedtime = s_soundtime + dma.submission_chunk;
		}
#endif
	}
}

//==========================================================================
//
//	S_Update_
//
//==========================================================================

static void S_Update_()
{
	static float	lastTime = 0.0f;
	static int		ot = -1;
	
	int				samps;

	if (!s_soundStarted || s_soundMuted)
	{
		return;
	}

	float thisTime = Com_Milliseconds();

	//	Updates s_soundtime
	GetSoundtime();

	// check to make sure that we haven't overshot
	if (!(GGameType & GAME_Quake3) && s_paintedtime < s_soundtime)
	{
		Log::develWrite("S_Update_ : overflow\n");
		s_paintedtime = s_soundtime;
	}

	float ma = s_mixahead->value * dma.speed;
	if (GGameType & GAME_Quake3)
	{
#ifdef _WIN32
		if (s_soundtime == ot)
		{
			return;
		}
#endif
		ot = s_soundtime;

		// clear any sound effects that end before the current time,
		// and start any new sounds
		S_ScanChannelStarts();

		float sane = thisTime - lastTime;
		if (sane < 11)
		{
			sane = 11;			// 85hz
		}

		float op = s_mixPreStep->value + sane * dma.speed * 0.01;

		if (op < ma)
		{
			ma = op;
		}
	}

	// mix ahead of current position
	int endtime = s_soundtime + ma;

	if (!(GGameType & GAME_QuakeHexen))
	{
		// mix to an even submission block size
		endtime = (endtime + dma.submission_chunk - 1) & ~ (dma.submission_chunk  -1);
	}

	// never mix more than the complete buffer
	samps = dma.samples >> (dma.channels-1);
	if (endtime - s_soundtime > samps)
		endtime = s_soundtime + samps;

	SNDDMA_BeginPainting();

	S_PaintChannels(endtime);

	SNDDMA_Submit();

	lastTime = thisTime;
}

//==========================================================================
//
//	S_Update
//
//	Called once each time through the main loop
//
//==========================================================================

void S_Update()
{
	if (!s_soundStarted || s_soundMuted)
	{
		Log::develWrite("not started or muted\n");
		return;
	}

	// if the laoding plaque is up, clear everything
	// out to make sure we aren't looping a dirty
	// dma buffer while loading
	if (cls.disable_screen)
	{
		S_ClearSoundBuffer();
		return;
	}

	//
	// debugging output
	//
	if (s_show->integer == 2)
	{
		int total = 0;
		channel_t* ch = s_channels;
		for (int i = 0; i < MAX_CHANNELS; i++, ch++)
		{
			if (ch->sfx && (ch->leftvol || ch->rightvol))
			{
				Log::write("%3i %3i %s\n", ch->leftvol, ch->rightvol, ch->sfx->Name);
				total++;
			}
		}

		Log::write("----(%i)---- painted: %i\n", total, s_paintedtime);
	}

	// add raw data from streamed samples
	S_UpdateStreamingSounds();

	// mix some sound
	S_Update_();
}

//==========================================================================
//
//	S_ExtraUpdate
//
//==========================================================================

void S_ExtraUpdate()
{
	if (!s_soundStarted)
	{
		return;
	}
	if (snd_noextraupdate->value)
	{
		return;		// don't pollute timings
	}
	S_Update_();
}

//**************************************************************************
//	Console functions
//**************************************************************************

//==========================================================================
//
//	S_Play_f
//
//==========================================================================

static void S_Play_f()
{
	char		name[256];

	int i = 1;
	while (i < Cmd_Argc())
	{
		if (!String::RChr(Cmd_Argv(i), '.'))
		{
			String::Sprintf(name, sizeof(name), "%s.wav", Cmd_Argv(1));
		}
		else
		{
			String::NCpyZ(name, Cmd_Argv(i), sizeof(name));
		}
		sfxHandle_t h = S_RegisterSound(name);
		if (h)
		{
			if (GGameType & GAME_QuakeHexen)
			{
				S_StartSound(listener_origin, listener_number, 0, h);
			}
			else if (GGameType & GAME_Quake2)
			{
				S_StartSound(NULL, listener_number, 0, h);
			}
			else
			{
				S_StartLocalSound(h, Q3CHAN_LOCAL_SOUND);
			}
		}
		i++;
	}
}

//==========================================================================
//
//	S_PlayVol_f
//
//==========================================================================

static void S_PlayVol_f()
{
	char		name[256];

	int i = 1;
	while (i < Cmd_Argc())
	{
		if (!String::RChr(Cmd_Argv(i), '.'))
		{
			String::Sprintf(name, sizeof(name), "%s.wav", Cmd_Argv(1));
		}
		else
		{
			String::NCpyZ(name, Cmd_Argv(i), sizeof(name));
		}
		sfxHandle_t h = S_RegisterSound(name);
		float vol = String::Atof(Cmd_Argv(i + 1));
		if (h)
		{
			if (GGameType & GAME_QuakeHexen)
			{
				S_StartSound(listener_origin, listener_number, 0, h, vol);
			}
			else if (GGameType & GAME_Quake2)
			{
				S_StartSound(NULL, listener_number, 0, h, vol);
			}
			else
			{
				S_StartSound(NULL, listener_number, Q3CHAN_LOCAL_SOUND, h, vol);
			}
		}
		i += 2;
	}
}

//==========================================================================
//
//	S_SoundList_f
//
//==========================================================================

static void S_SoundList_f()
{
	int		i;
	sfx_t	*sfx;
	int		size, total;
	char	mem[2][16];

	String::Cpy(mem[0], "paged out");
	String::Cpy(mem[1], "resident ");
	total = 0;
	for (sfx=s_knownSfx, i=0 ; i<s_numSfx; i++, sfx++)
	{
		if (sfx->Name[0] == '*')
		{
			Log::write("  placeholder : %s\n", sfx->Name);
		}
		else
		{
			size = sfx->Length;
			total += size;
			if (sfx->LoopStart >= 0)
				Log::write("L");
			else
				Log::write(" ");
			Log::write("%6i : %s[%s]\n", size, sfx->Name, mem[sfx->InMemory]);
		}
	}
	Log::write("Total resident: %i\n", total);
}

//==========================================================================
//
//	S_Init
//
//==========================================================================

void S_Init()
{
	Log::write("\n------- sound initialization -------\n");

	if (GGameType & GAME_QuakeHexen)
	{
		bgmvolume = Cvar_Get("bgmvolume", "1", CVAR_ARCHIVE);
		if (GGameType & GAME_Hexen2)
		{
			bgmtype = Cvar_Get("bgmtype", "cd", CVAR_ARCHIVE);   // cd or midi
		}
		s_ambient_level = Cvar_Get("s_ambient_level", "0.3", 0);
		s_ambient_fade = Cvar_Get("s_ambient_fade", "100", 0);
		snd_noextraupdate = Cvar_Get("snd_noextraupdate", "0", 0);
	}
	s_volume = Cvar_Get("s_volume", "0.8", CVAR_ARCHIVE);
	s_musicVolume = Cvar_Get("s_musicvolume", "0.25", CVAR_ARCHIVE);
	s_doppler = Cvar_Get("s_doppler", (GGameType & GAME_Quake3) ? "1" : "0", CVAR_ARCHIVE);
	s_khz = Cvar_Get("s_khz", "44", CVAR_ARCHIVE);
	s_bits = Cvar_Get("s_bits", "16", CVAR_ARCHIVE);
	s_channels_cv = Cvar_Get("s_channels", "2", CVAR_ARCHIVE);
	s_mixahead = Cvar_Get("s_mixahead", "0.2", CVAR_ARCHIVE);
	if (GGameType & GAME_Quake3)
	{
		s_mixPreStep = Cvar_Get("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	}
	s_show = Cvar_Get("s_show", "0", CVAR_CHEAT);
	s_testsound = Cvar_Get("s_testsound", "0", CVAR_CHEAT);

	Cvar* cv = Cvar_Get("s_initsound", "1", 0);
	if (!cv->integer)
	{
		Log::write("not initializing.\n");
		Log::write("------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("playvol", S_PlayVol_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("s_list", S_SoundList_f);
	Cmd_AddCommand("s_info", S_SoundInfo_f);
	Cmd_AddCommand("s_stop", S_StopAllSounds);

	bool r = SNDDMA_Init();
	Log::write("------------------------------------\n");

	if (r)
	{
		s_soundStarted = 1;
		if (GGameType & GAME_Quake3)
		{
			s_soundMuted = 1;
			//s_numSfx = 0;
		}
		else
		{
			s_numSfx = 0;
		}

		Com_Memset(sfxHash, 0, sizeof(sfx_t*) * LOOP_HASH);

		s_soundtime = 0;
		s_paintedtime = 0;

		if (GGameType & GAME_QuakeHexen)
		{
			ambient_sfx[BSP29AMBIENT_WATER] = s_knownSfx + S_RegisterSound("ambience/water1.wav");
			ambient_sfx[BSP29AMBIENT_SKY] = s_knownSfx + S_RegisterSound("ambience/wind2.wav");
		}

		S_StopAllSounds();

		S_SoundInfo_f();
	}
}

//==========================================================================
//
//	S_Shutdown
//
//	Shutdown sound engine
//
//==========================================================================

void S_Shutdown()
{
	if (!s_soundStarted)
	{
		return;
	}

	SNDDMA_Shutdown();

	s_soundStarted = 0;

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("playvol");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("s_sound");
	Cmd_RemoveCommand("s_list");
	Cmd_RemoveCommand("s_info");

	if (GGameType & GAME_Quake2)
	{
		// free all sounds
		sfx_t* sfx = s_knownSfx;
		for (int i = 0; i < s_numSfx; i++, sfx++)
		{
			if (!sfx->Name[0])
			{
				continue;
			}
			if (sfx->Data)
			{
				delete[] sfx->Data;
			}
			Com_Memset(sfx, 0, sizeof(*sfx));
		}

		s_numSfx = 0;
	}
}

//==========================================================================
//
//	S_DisableSounds
//
//	Disables sounds until the next S_BeginRegistration.
//	This is called when the hunk is cleared and the sounds are no longer valid.
//
//==========================================================================

void S_DisableSounds()
{
	S_StopAllSounds();
	s_soundMuted = true;
}
#endif
