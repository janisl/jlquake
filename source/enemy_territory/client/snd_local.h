/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// snd_local.h -- private sound definations


#include "../../wolfclient/client.h"
#include "../../wolfclient/sound/local.h"
#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "snd_public.h"

typedef struct loopSound_s {
	vec3_t origin;
	vec3_t velocity;
	sfx_t       *sfx;
	int mergeFrame;
	qboolean active;
	qboolean kill;
	qboolean doppler;
	float dopplerScale;
	float oldDopplerScale;
	int framenum;
	float range;            //----(SA)	added
	int vol;
	qboolean loudUnderWater;    // (SA) set if this sound should be played at full vol even when under water (under water loop sound for ex.)
	int startTime, startSample;         // ydnar: so looping sounds can be out of phase
} loopSound_t;

//====================================================================

extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;

//----(SA)	some flags for queued music tracks
#define QUEUED_PLAY_ONCE    -1
#define QUEUED_PLAY_LOOPED  -2
#define QUEUED_PLAY_ONCE_SILENT -3  // when done it goes quiet
//----(SA)	end

typedef struct {
	vec3_t origin;
	qboolean fixedOrigin;
	int entityNum;
	int entityChannel;
	sfxHandle_t sfx;
	int flags;
	int volume;
} s_pushStack;

#define MAX_PUSHSTACK   64
#define LOOP_HASH       128
#define MAX_LOOP_SOUNDS 1024

// removed many statics into a common sound struct
typedef struct {
	sfx_t       *sfxHash[LOOP_HASH];
	int numLoopSounds;
	loopSound_t loopSounds[MAX_LOOP_SOUNDS];

	float volTarget;
	float volStart;
	int volTime1;
	int volTime2;
	float volFadeFrac;

	qboolean stopSounds;

	channel_t   *freelist;
	channel_t   *endflist;

	int s_numSfx;

	s_pushStack pushPop[MAX_PUSHSTACK];
	int tart;

	qboolean s_soundPainted;
	int s_clearSoundBuffer;

	int s_soundStarted;
	int s_soundMute;                // 0 - not muted, 1 - muted, 2 - no new sounds, but play out remaining sounds (so they can die if necessary)

	vec3_t entityPositions[MAX_GENTITIES_Q3];

	char nextMusicTrack[MAX_QPATH];         // extracted from CS_MUSIC_QUEUE //----(SA)	added
	int nextMusicTrackType;
} snd_t;

extern snd_t snd;   // globals for sound

extern Cvar   *s_nosound;
extern Cvar   *s_show;
extern Cvar   *s_mixahead;

extern Cvar   *s_separation;
extern Cvar   *s_currentMusic;    //----(SA)	added
extern Cvar   *s_debugMusic;      //----(SA)	added

void S_memoryLoad( sfx_t *sfx );
portable_samplepair_t *S_GetRawSamplePointer();

// spatializes a channel
void S_Spatialize( channel_t *ch );

extern float S_GetStreamingFade( streamingSound_t *ss );    //----(SA)	added
