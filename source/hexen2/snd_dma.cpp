// snd_dma.c -- main control for any streaming sound output device

/*
 * $Header: /H2 Mission Pack/SND_DMA.C 9     4/01/98 6:43p Jmonroe $
 */

#include "quakedef.h"
#include "../../libs/client/snd_local.h"

QCvar* bgmvolume;
QCvar* bgmtype;


/*
================
S_Init
================
*/
void S_Init (void)
{

	Con_Printf("\nSound Initialization\n");

	bgmvolume = Cvar_Get("bgmvolume", "1", CVAR_ARCHIVE);
	bgmtype = Cvar_Get("bgmtype", "cd", CVAR_ARCHIVE);   // cd or midi
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

	Cmd_AddCommand("play", S_Play_f);
	Cmd_AddCommand("playvol", S_PlayVol_f);
	Cmd_AddCommand("music", S_Music_f);
	Cmd_AddCommand("stopsound", S_StopAllSounds);
	Cmd_AddCommand("soundlist", S_SoundList_f);
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
