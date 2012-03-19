/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 * $Archive: /Wolf5/src/client/snd_dma.c $
 *
 *****************************************************************************/

#include "snd_local.h"
#include "client.h"

void S_Play_f( void );
void S_SoundList_f( void );
void S_Music_f( void );
void S_QueueMusic_f( void );
void S_StreamingSound_f( void );

void S_Update_Mix();
void S_StopAllSounds( void );
void S_UpdateStreamingSounds( void );
void S_SpatializeOrigin( vec3_t origin, int master_vol, float dist_mult, int *left_vol, int *right_vol, float range, int noAttenuation );
void S_Music_f( void );
void S_ThreadStartSoundEx(const vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfxHandle, int flags, int volume);

snd_t snd;  // globals for sound

extern int numStreamingSounds;
extern vec3_t entityPositions[MAX_GENTITIES_Q3];

// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define     SOUND_FULLVOLUME    80

#define     SOUND_ATTENUATE     0.0008f
#define     SOUND_RANGE_DEFAULT 1250

extern int s_soundStarted;
extern bool s_soundMuted;
extern bool s_soundPainted;

extern int listener_number;
extern vec3_t listener_origin;
extern vec3_t listener_axis[3];
extern int s_numSfx;
extern sfx_t       *sfxHash[LOOP_HASH];
extern loopSound_t loopSounds[MAX_LOOPSOUNDS];
extern char nextMusicTrack[MAX_QPATH];         // extracted from CS_MUSIC_QUEUE //----(SA)	added
extern int nextMusicTrackType;
extern int numLoopSounds;

extern Cvar      *s_show;
extern Cvar      *s_mixahead;
extern Cvar      *s_mixPreStep;
extern Cvar      *s_musicVolume;
Cvar      *s_currentMusic;    //----(SA)	added
Cvar      *s_separation;
extern Cvar      *s_doppler;
extern Cvar      *s_defaultsound; // (SA) added to silence the default beep sound if desired
extern Cvar      *cl_cacheGathering; // Ridah


void S_ChannelFree( channel_t *v );
channel_t*  S_ChannelMalloc();
void S_ChannelSetup();
void S_SoundInfo_f( void );

/*
================
S_Init
================
*/
void S_Init( void ) {
	Cvar  *cv;
	qboolean r;

	Com_Printf( "\n------- sound initialization -------\n" );

	s_mute = Cvar_Get( "s_mute", "0", CVAR_TEMP ); //----(SA)	added
	s_volume = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicVolume = Cvar_Get( "s_musicvolume", "0.25", CVAR_ARCHIVE );
	s_currentMusic = Cvar_Get( "s_currentMusic", "", CVAR_ROM );
	s_separation = Cvar_Get( "s_separation", "0.5", CVAR_ARCHIVE );
	s_doppler = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
	s_khz = Cvar_Get( "s_khz", "22", CVAR_ARCHIVE );
	s_mixahead = Cvar_Get( "s_mixahead", "0.5", CVAR_ARCHIVE );    //DAJ was 0.2
	s_debugMusic = Cvar_Get( "s_debugMusic", "0", CVAR_TEMP );

	s_mixPreStep = Cvar_Get( "s_mixPreStep", "0.05", CVAR_ARCHIVE );
	s_show = Cvar_Get( "s_show", "0", CVAR_CHEAT );
	s_testsound = Cvar_Get( "s_testsound", "0", CVAR_CHEAT );
	s_defaultsound = Cvar_Get( "s_defaultsound", "0", CVAR_ARCHIVE );
	// Ridah
	cl_cacheGathering = Cvar_Get( "cl_cacheGathering", "0", 0 );

	// fretn
	s_bits = Cvar_Get( "s_bits", "16", CVAR_LATCH2 | CVAR_ARCHIVE );
	s_channels_cv = Cvar_Get( "s_channels", "2", CVAR_LATCH2 | CVAR_ARCHIVE );

	cv = Cvar_Get( "s_initsound", "1", 0 );
	if ( !cv->integer ) {
		Com_Printf( "not initializing.\n" );
		Com_Printf( "------------------------------------\n" );
		return;
	}

	Cmd_AddCommand( "play", S_Play_f );
	Cmd_AddCommand( "music", S_Music_f );
	Cmd_AddCommand( "queuemusic", S_QueueMusic_f );
	Cmd_AddCommand( "streamingsound", S_StreamingSound_f );
	Cmd_AddCommand( "s_list", S_SoundList_f );
	Cmd_AddCommand( "s_info", S_SoundInfo_f );
	Cmd_AddCommand( "s_stop", S_StopAllSounds );

	r = SNDDMA_Init();
	Com_Printf( "------------------------------------\n" );

	if ( r ) {
		Com_Memset( &snd, 0, sizeof( snd ) );
//		Com_Memset(snd.sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

		s_soundStarted = 1;
		s_soundMuted = 1;
//		snd.s_numSfx = 0;
//		snd.volTarget = 1.0f;	// full volume
		snd.volTarget = 0.0f;   // full volume

		s_soundtime = 0;
		s_paintedtime = 0;

		S_StopAllSounds();

		S_SoundInfo_f();
		S_ChannelSetup();
	}

}

/*
================
S_Shutdown
================
*/
void S_Shutdown( void ) {
	if ( !s_soundStarted ) {
		return;
	}

	SNDDMA_Shutdown();

	s_soundStarted = 0;
	s_soundMuted = 1;

	Cmd_RemoveCommand( "play" );
	Cmd_RemoveCommand( "music" );
	Cmd_RemoveCommand( "stopsound" );
	Cmd_RemoveCommand( "soundlist" );
	Cmd_RemoveCommand( "soundinfo" );
}

/*
===================
S_DisableSounds

Disables sounds until the next S_BeginRegistration.
This is called when the hunk is cleared and the sounds
are no longer valid.
===================
*/
void S_DisableSounds( void ) {
	S_StopAllSounds();
	s_soundMuted = 1;
}

/*
=================
S_memoryLoad
=================
*/
void S_memoryLoad( sfx_t *sfx ) {
	// load the sound file
	if ( !S_LoadSound( sfx ) ) {
//		Com_Printf( S_COLOR_YELLOW "WARNING: couldn't load sound: %s\n", sfx->soundName );
		sfx->DefaultSound = qtrue;
	}
	sfx->InMemory = qtrue;
}

//=============================================================================

/*
==================
S_ClearSoundBuffer

If we are about to perform file access, clear the buffer
so sound doesn't stutter.
==================
*/
void S_ClearSoundBuffer( qboolean killStreaming ) {
	if ( !s_soundStarted ) {
		return;
	}

	if ( !s_soundPainted ) {  // RF, buffers are clear, no point clearing again
		return;
	}

	s_soundPainted = qfalse;

//	snd.s_clearSoundBuffer = 4;
	snd.s_clearSoundBuffer = 3;

	S_ClearSounds( killStreaming, qtrue );    // do this now since you might not be allowed to in a sec (no multi-threaeded)
}

/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds( void ) {
	int i;
	if ( !s_soundStarted ) {
		return;
	}

	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

//DAJ BUGFIX	for(i=0;i<numStreamingSounds;i++) {
	for ( i = 0; i < MAX_STREAMING_SOUNDS; i++ ) {   //DAJ numStreamingSounds can get bigger than the MAX array size
		if ( i == 0 ) {
			continue;   // ignore music
		}
		streamingSounds[i].kill = 1;
	}
	// stop the background music
	S_StopBackgroundTrack();

	S_ClearSoundBuffer( qtrue );

	S_UpdateThread();   // clear the stuff that needs to clear
}

/*
==============================================================

continuous looping sounds are added each frame

==============================================================
*/

/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds( void ) {
	int i, j, time;
	int left_total, right_total, left, right;
	channel_t   *ch;
	loopSound_t *loop, *loop2;
	static int loopFrame;

	numLoopChannels = 0;

	time = Sys_Milliseconds();

	loopFrame++;
	for ( i = 0 ; i < numLoopSounds ; i++ ) {
		loop = &loopSounds[i];
		if ( loop->mergeFrame == loopFrame ) {
			continue;   // already merged into an earlier sound
		}

		//if (loop->kill) {
		//	S_SpatializeOrigin( loop->origin, 127, &left_total, &right_total, loop->range);	// 3d
		//} else {
		S_SpatializeOrigin( loop->origin, 90, 1,  &left_total, &right_total, loop->range, false );    // sphere
		//}

		// adjust according to volume
		left_total = (int)( (float)loop->vol * (float)left_total / 256.0 );
		right_total = (int)( (float)loop->vol * (float)right_total / 256.0 );

		loop->sfx->LastTimeUsed = time;

		for ( j = ( i + 1 ); j < numLoopChannels ; j++ ) {
			loop2 = &loopSounds[j];
			if ( loop2->sfx != loop->sfx ) {
				continue;
			}
			loop2->mergeFrame = loopFrame;

			//if (loop2->kill) {
			//	S_SpatializeOrigin( loop2->origin, 127, &left, &right, loop2->range);	// 3d
			//} else {
			S_SpatializeOrigin( loop2->origin, 90, 1,  &left, &right, loop2->range, false );      // sphere
			//}

			// adjust according to volume
			left = (int)( (float)loop2->vol * (float)left / 256.0 );
			right = (int)( (float)loop2->vol * (float)right / 256.0 );

			loop2->sfx->LastTimeUsed = time;
			left_total += left;
			right_total += right;
		}
		if ( left_total == 0 && right_total == 0 ) {
			continue;       // not audible
		}

		// allocate a channel
		ch = &loop_channels[numLoopChannels];

		if ( left_total > 255 ) {
			left_total = 255;
		}
		if ( right_total > 255 ) {
			right_total = 255;
		}

		ch->master_vol = 127;
		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->sfx = loop->sfx;
		// RF, disabled doppler for looping sounds for now, since we are reverting to the old looping sound code
		ch->doppler = qfalse;
		//ch->doppler = loop->doppler;
		//ch->dopplerScale = loop->dopplerScale;
		//ch->oldDopplerScale = loop->oldDopplerScale;
		numLoopChannels++;
		if ( numLoopChannels == MAX_CHANNELS ) {
			i = numLoopSounds + 1;
		}
	}
}

//=============================================================================

/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize( int entityNum, const vec3_t head, vec3_t axis[3], int inwater ) {

	if ( !s_soundStarted || ( s_soundMuted == 1 ) ) {
		return;
	}

	listener_number = entityNum;
	VectorCopy( head, listener_origin );
	VectorCopy( axis[0], listener_axis[0] );
	VectorCopy( axis[1], listener_axis[1] );
	VectorCopy( axis[2], listener_axis[2] );
}

void S_ThreadRespatialize() {
	int i;
	channel_t   *ch;
	vec3_t origin;
	// update spatialization for dynamic sounds
	ch = s_channels;
	for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
		if ( !ch->sfx ) {
			continue;
		}
		// anything coming from the view entity will always be full volume
		if ( ch->entnum == listener_number ) {
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		} else {
			if ( ch->fixed_origin ) {
				VectorCopy( ch->origin, origin );
			} else {
				VectorCopy( entityPositions[ ch->entnum ], origin );
			}

			S_SpatializeOrigin( origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol, SOUND_RANGE_DEFAULT, false );
		}
	}
}

/*
========================
S_ScanChannelStarts

Returns qtrue if any new sounds were started since the last mix
========================
*/
qboolean S_ScanChannelStarts( void ) {
	channel_t       *ch;
	int i;
	qboolean newSamples;

	newSamples = qfalse;
	ch = s_channels;

	for ( i = 0; i < MAX_CHANNELS; i++, ch++ ) {
		if ( !ch->sfx ) {
			continue;
		}
		// if this channel was just started this frame,
		// set the sample count to it begins mixing
		// into the very first sample
		if ( ch->startSample == START_SAMPLE_IMMEDIATE && ch->threadReady == qtrue ) {
			ch->startSample = s_paintedtime;
			newSamples = qtrue;
			continue;
		}

		// if it is completely finished by now, clear it
		if ( ch->startSample + ( ch->sfx->Length ) <= s_paintedtime ) {
//----(SA)	got from TA sound.  correct?
//			Com_Memset(ch, 0, sizeof(*ch));
			S_ChannelFree( ch );
		}
	}

	return newSamples;
}

/*
============
S_Update

Called once each time through the main loop
============
*/

void S_Update( void ) {
	int i;
	int total;
	channel_t   *ch;

	if ( !s_soundStarted || ( s_soundMuted == 1 ) ) {
//		Com_DPrintf ("not started or muted\n");
		return;
	}

	//
	// debugging output
	//
	if ( s_show->integer == 2 ) {
		total = 0;
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS; i++, ch++ ) {
			if ( ch->sfx && ( ch->leftvol || ch->rightvol ) ) {
				Com_Printf( "%f %f %s\n", ch->leftvol, ch->rightvol, ch->sfx->Name );          // <- this is not thread safe
				total++;
			}
		}

		Com_Printf( "----(%i)---- painted: %i\n", total, s_paintedtime );
	}
	// add loopsounds
	S_AddLoopSounds();
	S_UpdateThread();
}

/*
==============
S_UpdateThread
==============
*/
void S_UpdateThread( void ) {

	if ( !s_soundStarted || ( s_soundMuted == 1 ) ) {
//		Com_DPrintf ("not started or muted\n");
		return;
	}

	// default to ZERO amplitude, overwrite if sound is playing
	memset( s_entityTalkAmplitude, 0, sizeof( s_entityTalkAmplitude ) );

	if ( snd.s_clearSoundBuffer ) {
		S_ClearSounds( qtrue, (qboolean)( snd.s_clearSoundBuffer >= 4 ) );    //----(SA)	modified
		snd.s_clearSoundBuffer = 0;
	} else {
		S_ThreadRespatialize();
		// add raw data from streamed samples
		S_UpdateStreamingSounds();
		// mix some sound
		S_Update_Mix();
	}
}
/*
============
S_GetSoundtime
============
*/
void S_GetSoundtime( void ) {
	int samplepos;
	static int buffers;
	static int oldsamplepos;
	int fullsamples;

	fullsamples = dma.samples / dma.channels;

	// it is possible to miscount buffers if it has wrapped twice between
	// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();
	if ( samplepos < oldsamplepos ) {
		buffers++;                  // buffer wrapped

		if ( s_paintedtime > 0x40000000 ) { // time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_StopAllSounds();
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers * fullsamples + samplepos / dma.channels;

#if 0
// check to make sure that we haven't overshot
	if ( s_paintedtime < s_soundtime ) {
		Com_DPrintf( "S_GetSoundtime : overflow\n" );
		s_paintedtime = s_soundtime;
	}
#endif

	if ( dma.submission_chunk < 256 ) {
		s_paintedtime = s_soundtime + s_mixPreStep->value * dma.speed;
	} else {
		s_paintedtime = s_soundtime + dma.submission_chunk;
	}
}

/*
============
S_Update_Mix
============
*/
void S_Update_Mix( void ) {
	unsigned endtime;
	int samps;            //, i;
	static float lastTime = 0.0f;
	float ma, op;
	float thisTime, sane;

	if ( !s_soundStarted || ( s_soundMuted == 1 ) ) {
		return;
	}

	// RF, this isn't used anymore, since it was causing timing problems with streaming sounds, since the
	// starting of the sound is delayed, it could cause streaming sounds to be cutoff, when the steaming sound was issued after
	// this sound
/*
	for(i=0;i<snd.tart;i++) {
		if (snd.pushPop[i].fixedOrigin) {
			S_ThreadStartSoundEx(snd.pushPop[i].origin, snd.pushPop[i].entityNum, snd.pushPop[i].entityChannel, snd.pushPop[i].sfx, snd.pushPop[i].flags );
		} else {
			S_ThreadStartSoundEx(NULL, snd.pushPop[i].entityNum, snd.pushPop[i].entityChannel, snd.pushPop[i].sfx, snd.pushPop[i].flags );
		}
	}

	snd.tart = 0;
*/
	s_soundPainted = qtrue;

	thisTime = Sys_Milliseconds();

	// Updates s_soundtime
	S_GetSoundtime();

	// clear any sound effects that end before the current time,
	// and start any new sounds
	S_ScanChannelStarts();

	sane = thisTime - lastTime;
	if ( sane < 11 ) {
		sane = 11;          // 85hz
	}

	ma = s_mixahead->value * dma.speed;
	op = s_mixPreStep->value + sane * dma.speed * 0.01;

	if ( op < ma ) {
		ma = op;
	}

	// mix ahead of current position
	endtime = s_soundtime + ma;

	// mix to an even submission block size
	endtime = ( endtime + dma.submission_chunk - 1 )
			  & ~( dma.submission_chunk - 1 );

	// never mix more than the complete buffer
	samps = dma.samples >> ( dma.channels - 1 );
	if ( endtime - s_soundtime > samps ) {
		endtime = s_soundtime + samps;
	}

//----(SA)	added
	// global volume fading

	// endtime or s_paintedtime or s_soundtime...
	if ( s_soundtime < snd.volTime2 ) {    // still has fading to do
		if ( s_soundtime > snd.volTime1 ) {    // has started fading
			snd.volFadeFrac = ( (float)( s_soundtime - snd.volTime1 ) / (float)( snd.volTime2 - snd.volTime1 ) );
			s_volCurrent = ( ( 1.0 - snd.volFadeFrac ) * snd.volStart + snd.volFadeFrac * snd.volTarget );
		} else {
			s_volCurrent = snd.volStart;
		}
	} else {
		s_volCurrent = snd.volTarget;
	}
//----(SA)	end


	SNDDMA_BeginPainting();
	S_PaintChannels( endtime );
	SNDDMA_Submit();

	lastTime = thisTime;
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play_f( void ) {
	int i;
	sfxHandle_t h;
	char name[256];

	i = 1;
	while ( i < Cmd_Argc() ) {
		if ( !String::RChr( Cmd_Argv( i ), '.' ) ) {
			String::Sprintf( name, sizeof( name ), "%s.wav", Cmd_Argv( 1 ) );
		} else {
			String::NCpyZ( name, Cmd_Argv( i ), sizeof( name ) );
		}
		h = S_RegisterSound( name);
		if ( h ) {
			S_StartLocalSound( h, Q3CHAN_LOCAL_SOUND, 127 );
		}
		i++;
	}
}

/*
==============
S_QueueMusic_f
	console interface really just for testing
==============
*/
void S_QueueMusic_f( void ) {
	int type = -2;  // default to setting this as the next continual loop
	int c;

	c = Cmd_Argc();

	if ( c == 3 ) {
		type = String::Atoi( Cmd_Argv( 2 ) );
	}

	if ( type != -1 ) { // clamp to valid values (-1, -2)
		type = -2;
	}

	// NOTE: could actually use this to touch the file now so there's not a hit when the queue'd music is played?
	S_StartBackgroundTrack( Cmd_Argv( 1 ), Cmd_Argv( 1 ), type );
}

// Ridah, just for testing the streaming sounds
void S_StreamingSound_f( void ) {
	int c;

	c = Cmd_Argc();

	if ( c == 2 ) {
		S_StartStreamingSound( Cmd_Argv( 1 ), 0, -1, 0, 0 );
	} else if ( c == 5 ) {
		S_StartStreamingSound( Cmd_Argv( 1 ), 0, String::Atoi( Cmd_Argv( 2 ) ), String::Atoi( Cmd_Argv( 3 ) ), String::Atoi( Cmd_Argv( 4 ) ) );
	} else {
		Com_Printf( "streamingsound <soundfile> [entnum channel attenuation]\n" );
		return;
	}

}

void S_SoundList_f( void ) {
	int i;
	sfx_t   *sfx;
	int size, total;
	char mem[2][16];

	String::Cpy( mem[0], "paged out" );
	String::Cpy( mem[1], "resident " );
	total = 0;
	for ( sfx = s_knownSfx, i = 0 ; i < s_numSfx ; i++, sfx++ ) {
		size = sfx->Length;
		total += size;
		Com_Printf( "%6i : %s[%s]\n", size, sfx->Name, mem[sfx->InMemory] );
	}
	Com_Printf( "Total resident: %i\n", total );
}

/*
==============
S_FadeAllSounds

==============
*/
void S_FadeAllSounds( float targetVol, int time ) {

	snd.volStart = s_volCurrent;
	snd.volTarget = targetVol;

	snd.volTime1 = s_soundtime;
	snd.volTime2 = s_soundtime + ( ( (float)( dma.speed ) / 1000.0f ) * time );

	// instant
	if ( !time ) {
		snd.volTarget = snd.volStart = s_volCurrent = targetVol;  // set it
		snd.volTime1 = snd.volTime2 = 0;    // no fading
	}
}

void CL_WriteWaveFilePacket(int endtime)
{
}

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	return NULL;
}

int S_GetClFrameServertime()
{
	return 0;
}
