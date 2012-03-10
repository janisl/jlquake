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

#ifndef _SND_LOCAL_H
#define _SND_LOCAL_H

#define WAV_FORMAT_PCM			1

//#define MAX_CHANNELS			96
#define MAX_CHANNELS			128

// MAX_SFX may be larger than MAX_SOUNDS because
// of custom player sounds
#define MAX_SFX					4096

#define MAX_RAW_SAMPLES			16384

struct portable_samplepair_t
{
	int			left;	// the final values will be clamped to +/- 0x00ffff00 and shifted down
	int			right;
};

#if 0
struct sfx_t
{
	char			Name[MAX_QPATH];
	int 			Length;
	int 			LoopStart;
	short*			Data;
	int				RegistrationSequence;
	char*			TrueName;
	bool			DefaultSound;			// couldn't be loaded, so use buzz
	bool			InMemory;				// not in Memory
	int				LastTimeUsed;
	sfx_t*			HashNext;
};
#endif

struct dma_t
{
	int			channels;
	int			samples;				// mono samples in buffer
	int			submission_chunk;		// don't mix less than this #
	int			samplepos;				// in mono samples
	int			samplebits;
	int			speed;
	byte		*buffer;
};

#if 0
struct channel_t
{
	//	First part is also used to store ponter to next free channel, so
	// pointer of sfx must be somewhere after.
	int			allocTime;
	int			startSample;	// START_SAMPLE_IMMEDIATE = set immediately on next mix
	int			entnum;			// to allow overriding a specific sound
	int			entchannel;		// to allow overriding a specific sound
	sfx_t*		sfx;			// sfx structure
	int			leftvol;		// 0-255 volume after spatialization
	int			rightvol;		// 0-255 volume after spatialization
	vec3_t		origin;			// only use if fixed_origin is set
	vec_t		dist_mult;		// distance multiplier (attenuation/clipK)
	int			master_vol;		// 0-255 volume before spatialization
	qboolean	fixed_origin;	// use origin instead of fetching entnum's origin
	float		dopplerScale;
	float		oldDopplerScale;
	qboolean	doppler;
};

struct wavinfo_t
{
	int			format;
	int			rate;
	int			width;
	int			channels;
	int			loopstart;
	int			samples;
	int			dataofs;		// chunk starts this many bytes from file start
};

// a playsound_t will be generated by each call to S_StartSound,
// when the mixer reaches playsound->begin, the playsound will
// be assigned to a channel
struct playsound_t
{
	playsound_t*	prev;
	playsound_t*	next;
	sfx_t*			sfx;
	float			volume;
	float			attenuation;
	int				entnum;
	int				entchannel;
	qboolean		fixed_origin;	// use origin field instead of entnum's origin
	vec3_t			origin;
	int				begin;			// begin on this sample
};

sfx_t* S_FindName(const char* name, bool create = true);
sfx_t* S_AliasName(const char* aliasname, const char* truename);
void S_IssuePlaysound(playsound_t* ps);

bool S_LoadSound(sfx_t* sfx);

void S_PaintChannels(int endtime);
#endif

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init();
// shutdown the DMA xfer.
void SNDDMA_Shutdown();
// gets the current DMA position
int SNDDMA_GetDMAPos();
void SNDDMA_BeginPainting();
void SNDDMA_Submit();

extern dma_t					dma;

#if 0
extern "C"
{
extern int*						snd_p;
extern int						snd_linear_count;
extern short*					snd_out;
}
#endif

extern int						s_soundtime;
extern int   					s_paintedtime;

extern Cvar*					s_testsound;
extern Cvar*					s_khz;
extern Cvar*					s_bits;
extern Cvar*					s_channels_cv;

#if 0
extern channel_t				s_channels[MAX_CHANNELS];
extern channel_t				loop_channels[MAX_CHANNELS];
extern int						numLoopChannels;

extern playsound_t				s_pendingplays;

extern int						s_rawend;
extern portable_samplepair_t	s_rawsamples[MAX_RAW_SAMPLES];

extern sfx_t					s_knownSfx[MAX_SFX];
#endif

extern bool s_use_custom_memset;

#endif
