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


/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 * $Archive: /Wolfenstein MP/src/client/snd_dma.c $
 *
 *****************************************************************************/

#include "snd_local.h"
#include "client.h"

void S_Play_f( void );
void S_SoundList_f( void );
void S_Music_f( void );
void S_StreamingSound_f( void );

void S_Music_f( void );

// =======================================================================
// Internal sound data & structures
// =======================================================================

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define     SOUND_FULLVOLUME    80

#define     SOUND_ATTENUATE     0.0008f
#define     SOUND_RANGE_DEFAULT 1250

extern int s_soundStarted;
extern bool s_soundMuted;

extern int s_numSfx;

#define     LOOP_HASH       128
extern sfx_t       *sfxHash[LOOP_HASH];

extern Cvar      *s_show;
extern Cvar      *s_mixahead;
extern Cvar      *s_mixPreStep;
extern Cvar      *s_musicVolume;
Cvar      *s_separation;
extern Cvar      *s_doppler;
extern Cvar      *s_defaultsound; // (SA) added to silence the default beep sound if desired
extern Cvar      *cl_cacheGathering; // Ridah
extern Cvar   *s_debugMusic;      //----(SA)	added

extern float volTarget;

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
	s_separation = Cvar_Get( "s_separation", "0.5", CVAR_ARCHIVE );
	s_doppler = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
	s_khz = Cvar_Get( "s_khz", "22", CVAR_ARCHIVE );
	s_mixahead = Cvar_Get( "s_mixahead", "0.2", CVAR_ARCHIVE );
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
	Cmd_AddCommand( "streamingsound", S_StreamingSound_f );
	Cmd_AddCommand( "s_list", S_SoundList_f );
	Cmd_AddCommand( "s_info", S_SoundInfo_f );
	Cmd_AddCommand( "s_stop", S_StopAllSounds );

	r = SNDDMA_Init();
	Com_Printf( "------------------------------------\n" );

	if ( r ) {
		Com_Memset( sfxHash, 0, sizeof( sfx_t * ) * LOOP_HASH );

		s_soundStarted = 1;
		s_soundMuted = 1;
//		s_numSfx = 0;
		volTarget = 1;

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
	s_soundMuted = qtrue;

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
	s_soundMuted = qtrue;
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
