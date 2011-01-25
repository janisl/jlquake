/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// snd_dma.c -- main control for any streaming sound output device

#include "client.h"
#include "snd_loc.h"

void S_Play(void);
void S_SoundList(void);
void S_Update_();
void S_StopAllSounds(void);


// =======================================================================
// Internal sound data & structures
// =======================================================================

#define		SOUND_LOOPATTENUATE	0.003

// ====================================================================
// User-setable variables
// ====================================================================



/*
================
S_Init
================
*/
void S_Init (void)
{
	QCvar	*cv;

	Com_Printf("\n------- sound initialization -------\n");

	s_volume = Cvar_Get ("s_volume", "0.7", CVAR_ARCHIVE);
	s_musicVolume = Cvar_Get ("s_musicvolume", "0.25", CVAR_ARCHIVE);
	s_doppler = Cvar_Get ("s_doppler", "0", CVAR_ARCHIVE);
	s_khz = Cvar_Get ("s_khz", "11", CVAR_ARCHIVE);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", 0);
	s_testsound = Cvar_Get ("s_testsound", "0", 0);

	cv = Cvar_Get ("s_initsound", "1", 0);
	if (!cv->value)
	{
		Com_Printf ("not initializing.\n");
		Com_Printf("------------------------------------\n");
		return;
	}

	Cmd_AddCommand("play", S_Play);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("stopsound", S_StopAllSounds);
	Cmd_AddCommand("soundlist", S_SoundList);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);

	bool r = SNDDMA_Init();
	Com_Printf("------------------------------------\n");

	if (r)
	{
		s_soundStarted = 1;
		s_numSfx = 0;

		s_soundtime = 0;
		s_paintedtime = 0;

		Com_Memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

		S_StopAllSounds ();

		S_SoundInfo_f();
	}
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown(void)
{
	int		i;
	sfx_t	*sfx;

	if (!s_soundStarted)
		return;

	SNDDMA_Shutdown();

	s_soundStarted = 0;

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("music");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");

	// free all sounds
	for (i=0, sfx=s_knownSfx; i < s_numSfx; i++,sfx++)
	{
		if (!sfx->Name[0])
			continue;
		if (sfx->Data)
			delete[] sfx->Data;
		Com_Memset(sfx, 0, sizeof(*sfx));
	}

	s_numSfx = 0;
}

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	int				n;
	char			*p;
	sfx_t*			sfx;
	fileHandle_t	f;
	char			model[MAX_QPATH];
	char			sexedFilename[MAX_QPATH];
	char			maleFilename[MAX_QPATH];

	// determine what model the client is using
	model[0] = 0;
	entity_state_t *ent = &cl_entities[entnum].current;
	n = CS_PLAYERSKINS + ent->number - 1;
	if (cl.configstrings[n][0])
	{
		p = strchr(cl.configstrings[n], '\\');
		if (p)
		{
			p += 1;
			QStr::Cpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}
	// if we can't figure it out, they're male
	if (!model[0])
		QStr::Cpy(model, "male");

	// see if we already know of the model specific sound
	QStr::Sprintf (sexedFilename, sizeof(sexedFilename), "#players/%s/%s", model, base+1);
	sfx = S_FindName (sexedFilename, false);

	if (!sfx)
	{
		// no, so see if it exists
		FS_FOpenFileRead(&sexedFilename[1], &f, false);
		if (f)
		{
			// yes, close the file and register it
			FS_FCloseFile (f);
			sfx = s_knownSfx + S_RegisterSound (sexedFilename);
		}
		else
		{
			// no, revert to the male sound in the pak0.pak
			QStr::Sprintf (maleFilename, sizeof(maleFilename), "player/%s/%s", "male", base+1);
			sfx = S_AliasName (sexedFilename, maleFilename);
		}
	}

	return sfx;
}


// =======================================================================
// Start a sound effect
// =======================================================================

/*
==================
S_AddLoopSounds

Entities with a ->sound field will generated looped sounds
that are automatically started, stopped, and merged together
as the entities are sent to the client
==================
*/
void S_AddLoopSounds (void)
{
	int			i, j;
	int			sounds[MAX_EDICTS];
	int			left, right, left_total, right_total;
	channel_t	*ch;
	sfx_t		*sfx;
	int			num;
	entity_state_t	*ent;

	if (cl_paused->value)
		return;

	if (cls.state != ca_active)
		return;

	if (!cl.sound_prepped)
		return;

	numLoopChannels = 0;

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];
		sounds[i] = ent->sound;
	}

	for (i=0 ; i<cl.frame.num_entities ; i++)
	{
		if (!sounds[i])
			continue;

		sfxHandle_t Handle = cl.sound_precache[sounds[i]];
		if (Handle < 0 || Handle >= s_numSfx)
			continue;		// bad sound effect
		sfx = s_knownSfx + Handle;
		if (!sfx->Data)
			continue;

		num = (cl.frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &cl_parse_entities[num];

		// find the total contribution of all sounds of this type
		S_SpatializeOrigin (ent->origin, 255.0, SOUND_LOOPATTENUATE,
			&left_total, &right_total);
		for (j=i+1 ; j<cl.frame.num_entities ; j++)
		{
			if (sounds[j] != sounds[i])
				continue;
			sounds[j] = 0;	// don't check this again later

			num = (cl.frame.parse_entities + j)&(MAX_PARSE_ENTITIES-1);
			ent = &cl_parse_entities[num];

			S_SpatializeOrigin (ent->origin, 255.0, SOUND_LOOPATTENUATE, 
				&left, &right);
			left_total += left;
			right_total += right;
		}

		if (left_total == 0 && right_total == 0)
			continue;		// not audible

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

		ch->leftvol = left_total;
		ch->rightvol = right_total;
		ch->sfx = sfx;
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
S_Update

Called once each time through the main loop
============
*/
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	int			i;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if (!s_soundStarted)
		return;

	// if the laoding plaque is up, clear everything
	// out to make sure we aren't looping a dirty
	// dma buffer while loading
	if (cls.disable_screen)
	{
		S_ClearSoundBuffer ();
		return;
	}

	listener_number = cl.playernum + 1;
	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_axis[0]);
	VectorSubtract(vec3_origin, right, listener_axis[1]);
	VectorCopy(up, listener_axis[2]);

	combine = NULL;

	if (cls.state == ca_active)
	{
		vec3_t		origin;

		for (i = 0; i < MAX_EDICTS; i++)
		{
			CL_GetEntitySoundOrigin(i, origin);
			S_UpdateEntityPosition(i, origin);
		}
	}

	// update spatialization for dynamic sounds	
	ch = s_channels;
	for (i=0 ; i<MAX_CHANNELS; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		S_Spatialize(ch);         // respatialize channel
		if (!ch->leftvol && !ch->rightvol)
		{
			Com_Memset(ch, 0, sizeof(*ch));
			continue;
		}
	}

	// add loopsounds
	S_AddLoopSounds ();

	//
	// debugging output
	//
	if (s_show->value)
	{
		total = 0;
		ch = s_channels;
		for (i=0 ; i<MAX_CHANNELS; i++, ch++)
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				Com_Printf ("%3i %3i %s\n", ch->leftvol, ch->rightvol, ch->sfx->Name);
				total++;
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

void S_Play(void)
{
	int 	i;
	char name[256];
	sfxHandle_t	sfx;
	
	i = 1;
	while (i<Cmd_Argc())
	{
		if (!QStr::RChr(Cmd_Argv(i), '.'))
		{
			QStr::Cpy(name, Cmd_Argv(i));
			QStr::Cat(name, sizeof(name), ".wav");
		}
		else
			QStr::Cpy(name, Cmd_Argv(i));
		sfx = S_RegisterSound(name);
		S_StartSound(NULL, listener_number, 0, sfx, 1.0, 1.0, 0);
		i++;
	}
}

void S_SoundList(void)
{
	int		i;
	sfx_t	*sfx;
	int		size, total;

	total = 0;
	for (sfx=s_knownSfx, i=0 ; i<s_numSfx ; i++, sfx++)
	{
		if (!sfx->RegistrationSequence)
			continue;
		if (sfx->Data)
		{
			size = sfx->Length * 2;
			total += size;
			if (sfx->LoopStart >= 0)
				Com_Printf ("L");
			else
				Com_Printf (" ");
			Com_Printf(" %6i : %s\n", size, sfx->Name);
		}
		else
		{
			if (sfx->Name[0] == '*')
				Com_Printf("  placeholder : %s\n", sfx->Name);
			else
				Com_Printf("  not loaded  : %s\n", sfx->Name);
		}
	}
	Com_Printf ("Total resident: %i\n", total);
}

int S_GetClientFrameCount()
{
	return cls.framecount;
}

int S_GetClFrameServertime()
{
	return cl.frame.servertime;
}
