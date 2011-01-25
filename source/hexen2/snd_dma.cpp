// snd_dma.c -- main control for any streaming sound output device

/*
 * $Header: /H2 Mission Pack/SND_DMA.C 9     4/01/98 6:43p Jmonroe $
 */

#include "quakedef.h"
#include "../../libs/client/snd_local.h"

void S_Play(void);
void S_PlayVol(void);
void S_SoundList(void);
void S_Update_();

// =======================================================================
// Internal sound data & structures
// =======================================================================

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

//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
void S_UpdateAmbientSounds (void)
{
	float		vol;
	int			ambient_channel;
	channel_t	*chan;

	// calc ambient sound levels
	byte* ambient_sound_level = CM_LeafAmbientSoundLevel(CM_PointLeafnum(listener_origin));
	if (!ambient_sound_level || !ambient_level->value)
	{
		for (ambient_channel = 0 ; ambient_channel< BSP29_NUM_AMBIENTS ; ambient_channel++)
			loop_channels[ambient_channel].sfx = NULL;
		return;
	}

	for (ambient_channel = 0 ; ambient_channel< BSP29_NUM_AMBIENTS ; ambient_channel++)
	{
		chan = &loop_channels[ambient_channel];	
		chan->sfx = ambient_sfx[ambient_channel];

		vol = ambient_level->value * ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += host_frametime * ambient_fade->value;
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= host_frametime * ambient_fade->value;
			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}
		
		chan->leftvol = chan->rightvol = chan->master_vol;
	}
}


/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	int			i, j;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if (!s_soundStarted || s_soundMuted)
		return;

	listener_number = cl.viewentity;
	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_axis[0]);
	VectorSubtract(vec3_origin, right, listener_axis[1]);
	VectorCopy(up, listener_axis[2]);
	
// update general area ambient sound sources
	S_UpdateAmbientSounds ();

	combine = NULL;

// update spatialization for static and dynamic sounds	
	ch = s_channels;
	for (i=0; i<MAX_CHANNELS; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		S_Spatialize(ch);         // respatialize channel
	}

// update spatialization for static and dynamic sounds	
	ch = loop_channels + BSP29_NUM_AMBIENTS;
	for (i=BSP29_NUM_AMBIENTS; i<numLoopChannels; i++, ch++)
	{
		if (!ch->sfx)
			continue;
		S_Spatialize(ch);         // respatialize channel
		if (!ch->leftvol && !ch->rightvol)
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame
	
	// see if it can just use the last one
		if (combine && combine->sfx == ch->sfx)
		{
			combine->leftvol += ch->leftvol;
			combine->rightvol += ch->rightvol;
			ch->leftvol = ch->rightvol = 0;
			continue;
		}
	// search for one
		combine = loop_channels;
		for (j=BSP29_NUM_AMBIENTS; j<i; j++, combine++)
			if (combine->sfx == ch->sfx)
				break;
				
		if (j == numLoopChannels)
		{
			combine = NULL;
		}
		else
		{
			if (combine != ch)
			{
				combine->leftvol += ch->leftvol;
				combine->rightvol += ch->rightvol;
				ch->leftvol = ch->rightvol = 0;
			}
			continue;
		}
	}

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
				Con_Printf ("%s %3i %3i\n", ch->sfx->Name, ch->leftvol, ch->rightvol);
				total++;
			}
		ch = loop_channels;
		for (i=0 ; i<numLoopChannels; i++, ch++)
			if (ch->sfx && (ch->leftvol || ch->rightvol) )
			{
				Con_Printf ("%s %3i %3i\n", ch->sfx->Name, ch->leftvol, ch->rightvol);
				total++;
			}
		
		Con_Printf ("----(%i)----\n", total);
	}

	// add raw data from streamed samples
	S_UpdateBackgroundTrack();

// mix some sound
	S_Update_();
}

void S_ExtraUpdate (void)
{
	if (snd_noextraupdate->value)
		return;		// don't pollute timings
	S_Update_();
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

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	return NULL;
}

int S_GetClFrameServertime()
{
	return 0;
}
