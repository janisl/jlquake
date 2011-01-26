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

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("stopsound", S_StopAllSounds);
	Cmd_AddCommand("soundlist", S_SoundList_f);
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

int S_GetClientFrameCount()
{
	return cls.framecount;
}

float S_GetClientFrameTime()
{
	return cls.frametime;
}

int S_GetClFrameServertime()
{
	return cl.frame.servertime;
}

byte* CM_LeafAmbientSoundLevel(int LeafNum)
{
	return NULL;
}

bool S_GetDisableScreen()
{
	return !!cls.disable_screen;
}
