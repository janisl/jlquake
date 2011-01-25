/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		snd_dma.c
 *
 * desc:		main control for any streaming sound output device
 *
 * $Archive: /MissionPack/code/client/snd_dma.c $
 *
 *****************************************************************************/

#include "client.h"
#include "../../../libs/client/snd_local.h"

void S_Play_f(void);
void S_SoundList_f(void);

void S_Update_();
void S_StopAllSounds(void);

// =======================================================================
// Internal sound data & structures
// =======================================================================

#define		SOUND_ATTENUATE		0.0008f


// ====================================================================
// User-setable variables
// ====================================================================


/*
================
S_Init
================
*/
void S_Init( void ) {
	QCvar	*cv;
	qboolean	r;

	Com_Printf("\n------- sound initialization -------\n");

	s_volume = Cvar_Get ("s_volume", "0.8", CVAR_ARCHIVE);
	s_musicVolume = Cvar_Get ("s_musicvolume", "0.25", CVAR_ARCHIVE);
	s_doppler = Cvar_Get ("s_doppler", "1", CVAR_ARCHIVE);
	s_khz = Cvar_Get ("s_khz", "22", CVAR_ARCHIVE);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);

	s_mixPreStep = Cvar_Get ("s_mixPreStep", "0.05", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", CVAR_CHEAT);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);

	cv = Cvar_Get ("s_initsound", "1", 0);
	if (!cv->integer)
	{
		Com_Printf("not initializing.\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("s_list", S_SoundList_f);
	Cmd_AddCommand("s_info", S_SoundInfo_f);
	Cmd_AddCommand("s_stop", S_StopAllSounds);

	r = SNDDMA_Init();
	Com_Printf("------------------------------------\n");

	if ( r ) {
		s_soundStarted = 1;
		s_soundMuted = 1;
//		s_numSfx = 0;

		Com_Memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

		s_soundtime = 0;
		s_paintedtime = 0;

		S_StopAllSounds ();

		S_SoundInfo_f();
	}

}

// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown( void ) {
	if ( !s_soundStarted ) {
		return;
	}

	SNDDMA_Shutdown();

	s_soundStarted = 0;

    Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");
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

// =======================================================================
// Start a sound effect
// =======================================================================

/*
==================
S_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void S_AddLoopSounds (void) {
	int			i, j, time;
	int			left_total, right_total, left, right;
	channel_t	*ch;
	loopSound_t	*loop, *loop2;
	static int	loopFrame;


	numLoopChannels = 0;

	time = Com_Milliseconds();

	loopFrame++;
	for ( i = 0 ; i < MAX_GENTITIES ; i++) {
		loop = &loopSounds[i];
		if ( !loop->active || loop->mergeFrame == loopFrame ) {
			continue;	// already merged into an earlier sound
		}

		if (loop->kill) {
			S_SpatializeOrigin( loop->origin, 127, SOUND_ATTENUATE, &left_total, &right_total);			// 3d
		} else {
			S_SpatializeOrigin( loop->origin, 90,  SOUND_ATTENUATE, &left_total, &right_total);			// sphere
		}

		loop->sfx->LastTimeUsed = time;

		for (j=(i+1); j< MAX_GENTITIES ; j++) {
			loop2 = &loopSounds[j];
			if ( !loop2->active || loop2->doppler || loop2->sfx != loop->sfx) {
				continue;
			}
			loop2->mergeFrame = loopFrame;

			if (loop2->kill) {
				S_SpatializeOrigin( loop2->origin, 127, SOUND_ATTENUATE, &left, &right);				// 3d
			} else {
				S_SpatializeOrigin( loop2->origin, 90,  SOUND_ATTENUATE, &left, &right);				// sphere
			}

			loop2->sfx->LastTimeUsed = time;
			left_total += left;
			right_total += right;
		}
		if (left_total == 0 && right_total == 0) {
			continue;		// not audible
		}

		// allocate a channel
		ch = &loop_channels[numLoopChannels];
		
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

//=============================================================================


/*
============
S_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void S_Respatialize( int entityNum, const vec3_t head, vec3_t axis[3], int inwater )
{
	int			i;
	channel_t	*ch;
	vec3_t		origin;

	if ( !s_soundStarted || s_soundMuted ) {
		return;
	}

	listener_number = entityNum;
	VectorCopy(head, listener_origin);
	VectorCopy(axis[0], listener_axis[0]);
	VectorCopy(axis[1], listener_axis[1]);
	VectorCopy(axis[2], listener_axis[2]);

	// update spatialization for dynamic sounds	
	ch = s_channels;
	for ( i = 0 ; i < MAX_CHANNELS ; i++, ch++ ) {
		if ( !ch->sfx ) {
			continue;
		}
		// anything coming from the view entity will always be full volume
		if (ch->entnum == listener_number) {
			ch->leftvol = ch->master_vol;
			ch->rightvol = ch->master_vol;
		} else {
			if (ch->fixed_origin) {
				VectorCopy( ch->origin, origin );
			} else {
				VectorCopy( loopSounds[ ch->entnum ].origin, origin );
			}

			S_SpatializeOrigin (origin, ch->master_vol, SOUND_ATTENUATE, &ch->leftvol, &ch->rightvol);
		}
	}

	// add loopsounds
	S_AddLoopSounds ();
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update( void ) {
	int			i;
	int			total;
	channel_t	*ch;

	if ( !s_soundStarted || s_soundMuted ) {
		Com_DPrintf ("not started or muted\n");
		return;
	}

	//
	// debugging output
	//
	if ( s_show->integer == 2 ) {
		total = 0;
		ch = s_channels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++) {
			if (ch->sfx && (ch->leftvol || ch->rightvol) ) {
				Com_Printf ("%f %f %s\n", ch->leftvol, ch->rightvol, ch->sfx->Name);
				total++;
			}
		}
		
		Com_Printf ("----(%i)---- painted: %i\n", total, s_paintedtime);
	}

	// add raw data from streamed samples
	S_UpdateBackgroundTrack();

	// mix some sound
	S_Update_();
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play_f( void ) {
	int 		i;
	sfxHandle_t	h;
	char		name[256];
	
	i = 1;
	while ( i<Cmd_Argc() ) {
		if ( !QStr::RChr(Cmd_Argv(i), '.') ) {
			QStr::Sprintf( name, sizeof(name), "%s.wav", Cmd_Argv(1) );
		} else {
			QStr::NCpyZ( name, Cmd_Argv(i), sizeof(name) );
		}
		h = S_RegisterSound(name);
		if( h ) {
			S_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
		i++;
	}
}

void S_SoundList_f()
{
	int		i;
	sfx_t	*sfx;
	int		size, total;
	char	mem[2][16];

	QStr::Cpy(mem[0], "paged out");
	QStr::Cpy(mem[1], "resident ");
	total = 0;
	for (sfx=s_knownSfx, i=0 ; i<s_numSfx ; i++, sfx++) {
		size = sfx->Length;
		total += size;
		Com_Printf("%6i : %s[%s]\n", size, sfx->Name, mem[sfx->InMemory]);
	}
	Com_Printf ("Total resident: %i\n", total);
}

int S_GetClientFrameCount()
{
	return cls.framecount;
}

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	return NULL;
}

int S_GetClFrameServertime()
{
	return 0;
}
