/*
Copyright (C) 1996-1997 Id Software, Inc.

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

#include "quakedef.h"
#include "../../../libs/client/snd_local.h"

void S_Play(void);
void S_PlayVol(void);
void S_SoundList(void);
void S_Update_();

// =======================================================================
// Internal sound data & structures
// =======================================================================

QCvar* bgmvolume;


/*
================
S_Init
================
*/
void S_Init (void)
{

//	Con_Printf("\nSound Initialization\n");

	bgmvolume = Cvar_Get("bgmvolume", "1", CVAR_ARCHIVE);
	s_volume = Cvar_Get("volume", "0.7", CVAR_ARCHIVE);
	s_musicVolume = Cvar_Get ("s_musicvolume", "0.25", CVAR_ARCHIVE);
	nosound = Cvar_Get("nosound", "0", 0);
	ambient_level = Cvar_Get("ambient_level", "0.3", 0);
	ambient_fade = Cvar_Get("ambient_fade", "100", 0);
	snd_noextraupdate = Cvar_Get("snd_noextraupdate", "0", 0);
	s_show = Cvar_Get("s_show", "0", 0);
	s_mixahead = Cvar_Get("s_mixahead", "0.1", CVAR_ARCHIVE);
	s_testsound = Cvar_Get ("s_testsound", "0", CVAR_CHEAT);

	if (COM_CheckParm("-nosound"))
		return;

	Cmd_AddCommand("play", S_Play);
	Cmd_AddCommand("playvol", S_PlayVol);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("stopsound", S_StopAllSounds);
	Cmd_AddCommand("soundlist", S_SoundList);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);

	bool r = SNDDMA_Init();

	if (r)
	{
		s_soundStarted = 1;

		s_numSfx = 0;

		ambient_sfx[BSP29AMBIENT_WATER] = s_knownSfx + S_RegisterSound("ambience/water1.wav");
		ambient_sfx[BSP29AMBIENT_SKY] = s_knownSfx + S_RegisterSound("ambience/wind2.wav");

		Com_Memset(sfxHash, 0, sizeof(sfx_t *)*LOOP_HASH);

		S_StopAllSounds();

		S_SoundInfo_f();
	}
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_Shutdown(void)
{

	if (!s_soundStarted)
		return;

	s_soundStarted = 0;

	SNDDMA_Shutdown();
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play(void)
{
	static int hash=345;
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
		S_StartSound(listener_origin, hash++, 0, sfx, 1.0, 1.0);
		i++;
	}
}

void S_PlayVol(void)
{
	static int hash=543;
	int i;
	float vol;
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
		vol = QStr::Atof(Cmd_Argv(i+1));
		S_StartSound(listener_origin, hash++, 0, sfx, vol, 1.0);
		i+=2;
	}
}

void S_SoundList(void)
{
	int		i;
	sfx_t	*sfx;
	int		size, total;

	total = 0;
	for (sfx=s_knownSfx, i=0 ; i<s_numSfx; i++, sfx++)
	{
		if (!sfx->Data)
			continue;
		size = sfx->Length * 2;
		total += size;
		if (sfx->LoopStart >= 0)
			Con_Printf ("L");
		else
			Con_Printf (" ");
		Con_Printf(" %6i : %s\n", size, sfx->Name);
	}
	Con_Printf ("Total resident: %i\n", total);
}

int S_GetClientFrameCount()
{
	return host_framecount;
}

float S_GetClientFrameTime()
{
	return host_frametime;
}

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	return NULL;
}

int S_GetClFrameServertime()
{
	return 0;
}

bool S_GetDisableScreen()
{
	return false;
}
