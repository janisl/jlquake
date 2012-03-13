/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// snd_local.h -- private sound definations


#include "../../wolfclient/client.h"
#include "../../wolfclient/sound/local.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

#define PAINTBUFFER_SIZE        4096                    // this is in samples

#define SND_CHUNK_SIZE          1024                    // samples

//#define TALKANIM			// NERVE - SMF - we don't want this for multiplayer

typedef struct sndBuffer_s {
	short sndChunk[SND_CHUNK_SIZE];
	struct sndBuffer_s      *next;
	int size;
} sndBuffer;

struct sfx_t
{
	char Name[MAX_QPATH];
	int Length;
	bool DefaultSound;                  // couldn't be loaded, so use buzz
	bool InMemory;                      // not in Memory
	int LastTimeUsed;
	sfx_t* HashNext;

	sndBuffer       *soundData;
};

#define START_SAMPLE_IMMEDIATE  0x7fffffff

typedef struct loopSound_s {
	vec3_t origin;
	vec3_t velocity;
	float range;            //----(SA)	added
	sfx_t       *sfx;
	int mergeFrame;
	int vol;
	qboolean loudUnderWater;    // (SA) set if this sound should be played at full vol even when under water (under water loop sound for ex.)
	qboolean doppler;
	float dopplerScale;
	float oldDopplerScale;
	int framenum;
} loopSound_t;

typedef struct
{
	int         *prt;           //DAJ BUGFIX for freelist/endlist pointer
	int allocTime;
	int startSample;            // START_SAMPLE_IMMEDIATE = set immediately on next mix
	int entnum;                 // to allow overriding a specific sound
	int entchannel;             // to allow overriding a specific sound
	int leftvol;                // 0-255 volume after spatialization
	int rightvol;               // 0-255 volume after spatialization
	int master_vol;             // 0-255 volume before spatialization
	float dopplerScale;
	float oldDopplerScale;
	vec3_t origin;              // only use if fixed_origin is set
	qboolean fixed_origin;      // use origin instead of fetching entnum's origin
	sfx_t       *thesfx;        // sfx structure
	qboolean doppler;
	int flags;                  //----(SA)	added
	qboolean threadReady;
} channel_t;


typedef struct {
	int format;
	int rate;
	int width;
	int channels;
	int samples;
	int dataofs;                // chunk starts this many bytes from file start
} wavinfo_t;

//====================================================================

extern channel_t s_channels[MAX_CHANNELS];
extern channel_t loop_channels[MAX_CHANNELS];
extern int numLoopChannels;

extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;

#ifdef TALKANIM
extern unsigned char s_entityTalkAmplitude[MAX_CLIENTS_WM];
#endif

// Ridah, streaming sounds
typedef struct {
	fileHandle_t file;
	wavinfo_t info;
	int samples;
	char loop[MAX_QPATH];
	int entnum;
	int channel;
	int attenuation;
	qboolean kill;
} streamingSound_t;

#ifdef __MACOS__
#define MAX_STREAMING_SOUNDS    12  //DAJ use SP number (was 24)	// need to keep it low, or the rawsamples will get too big
#else
#define MAX_STREAMING_SOUNDS    24  // need to keep it low, or the rawsamples will get too big
#endif

extern streamingSound_t streamingSounds[MAX_STREAMING_SOUNDS];
extern int s_rawend[MAX_STREAMING_SOUNDS];
extern portable_samplepair_t s_rawsamples[MAX_STREAMING_SOUNDS][MAX_RAW_SAMPLES];
extern portable_samplepair_t s_rawVolume[MAX_STREAMING_SOUNDS];


extern Cvar   *s_nosound;
extern Cvar   *s_show;
extern Cvar   *s_mixahead;
extern Cvar   *s_mute;

extern Cvar   *s_separation;

qboolean S_LoadSound( sfx_t *sfx );

void        SND_free( sndBuffer *v );
sndBuffer*  SND_malloc();
void        SND_setup();

void S_PaintChannels( int endtime );

void S_memoryLoad( sfx_t *sfx );
portable_samplepair_t *S_GetRawSamplePointer();

// spatializes a channel
void S_Spatialize( channel_t *ch );

void S_FreeOldestSound();

extern short *sfxScratchBuffer;
extern const sfx_t *sfxScratchPointer;
extern int sfxScratchIndex;

