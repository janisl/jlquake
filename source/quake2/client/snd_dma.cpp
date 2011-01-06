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

// only begin attenuating sound volumes when outside the FULLVOLUME range
#define		SOUND_FULLVOLUME	80

#define		SOUND_LOOPATTENUATE	0.003

int			s_registration_sequence;

qboolean	s_registering;

#define		MAX_PLAYSOUNDS	128
playsound_t	s_playsounds[MAX_PLAYSOUNDS];
playsound_t	s_freeplays;

int			s_beginofs;

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


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
sfx_t *S_FindName (char *name, qboolean create)
{
	int		i;
	sfx_t	*sfx;

	if (!name)
		Com_Error (ERR_FATAL, "S_FindName: NULL\n");
	if (!name[0])
		Com_Error (ERR_FATAL, "S_FindName: empty name\n");

	if (QStr::Length(name) >= MAX_QPATH)
		Com_Error (ERR_FATAL, "Sound name too long: %s", name);

	// see if already loaded
	for (i=0 ; i < s_numSfx; i++)
		if (!QStr::Cmp(s_knownSfx[i].Name, name))
		{
			return &s_knownSfx[i];
		}

	if (!create)
		return NULL;

	// find a free sfx
	for (i=0 ; i < s_numSfx; i++)
		if (!s_knownSfx[i].Name[0])
//			registration_sequence < s_registration_sequence)
			break;

	if (i == s_numSfx)
	{
		if (s_numSfx == MAX_SFX)
			Com_Error (ERR_FATAL, "S_FindName: out of sfx_t");
		s_numSfx++;
	}
	
	sfx = &s_knownSfx[i];
	Com_Memset(sfx, 0, sizeof(*sfx));
	QStr::Cpy(sfx->Name, name);
	sfx->RegistrationSequence = s_registration_sequence;
	
	return sfx;
}


/*
==================
S_AliasName

==================
*/
sfx_t *S_AliasName (char *aliasname, char *truename)
{
	sfx_t	*sfx;
	char	*s;
	int		i;

	s = (char*)Z_Malloc (MAX_QPATH);
	QStr::Cpy(s, truename);

	// find a free sfx
	for (i=0 ; i < s_numSfx; i++)
		if (!s_knownSfx[i].Name[0])
			break;

	if (i == s_numSfx)
	{
		if (s_numSfx == MAX_SFX)
			Com_Error (ERR_FATAL, "S_FindName: out of sfx_t");
		s_numSfx++;
	}
	
	sfx = &s_knownSfx[i];
	Com_Memset(sfx, 0, sizeof(*sfx));
	QStr::Cpy(sfx->Name, aliasname);
	sfx->RegistrationSequence = s_registration_sequence;
	sfx->TrueName = s;

	return sfx;
}


/*
=====================
S_BeginRegistration

=====================
*/
void S_BeginRegistration (void)
{
	s_registration_sequence++;
	s_registering = true;
}

/*
==================
S_RegisterSound

==================
*/
sfx_t *S_RegisterSound (char *name)
{
	sfx_t	*sfx;

	if (!s_soundStarted)
		return NULL;

	sfx = S_FindName (name, true);
	sfx->RegistrationSequence = s_registration_sequence;

	if (!s_registering)
		S_LoadSound (sfx);

	return sfx;
}


/*
=====================
S_EndRegistration

=====================
*/
void S_EndRegistration (void)
{
	int		i;
	sfx_t	*sfx;
	int		size;

	// free any sounds not from this registration sequence
	for (i=0, sfx=s_knownSfx; i < s_numSfx; i++,sfx++)
	{
		if (!sfx->Name[0])
			continue;
		if (sfx->RegistrationSequence != s_registration_sequence)
		{	// don't need this sound
			if (sfx->Data)	// it is possible to have a leftover
				delete[] sfx->Data;	// from a server that didn't finish loading
			Com_Memset(sfx, 0, sizeof(*sfx));
		}
	}

	// load everything in
	for (i=0, sfx=s_knownSfx; i < s_numSfx; i++,sfx++)
	{
		if (!sfx->Name[0])
			continue;
		S_LoadSound (sfx);
	}

	s_registering = false;
}


//=============================================================================

/*
=================
S_PickChannel
=================
*/
channel_t *S_PickChannel(int entnum, int entchannel)
{
    int			ch_idx;
    int			first_to_die;
    int			life_left;
	channel_t	*ch;

	if (entchannel<0)
		Com_Error (ERR_DROP, "S_PickChannel: entchannel<0");

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx=0 ; ch_idx < MAX_CHANNELS ; ch_idx++)
    {
		if (entchannel != 0		// channel 0 never overrides
		&& s_channels[ch_idx].entnum == entnum
		&& s_channels[ch_idx].entchannel == entchannel)
		{	// always override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (s_channels[ch_idx].entnum == listener_number && entnum != listener_number && s_channels[ch_idx].sfx)
			continue;

		if (!s_channels[ch_idx].sfx)
		{
			if (life_left > 0)
			{
				life_left = 0;
				first_to_die = ch_idx;
			}
			continue;
		}

		if (s_channels[ch_idx].startSample + s_channels[ch_idx].sfx->Length - s_paintedtime < life_left)
		{
			life_left = s_channels[ch_idx].startSample + s_channels[ch_idx].sfx->Length - s_paintedtime;
			first_to_die = ch_idx;
		}
   }

	if (first_to_die == -1)
		return NULL;

	ch = &s_channels[first_to_die];
	Com_Memset(ch, 0, sizeof(*ch));

    return ch;
}       

/*
=================
S_SpatializeOrigin

Used for spatializing channels and autosounds
=================
*/
void S_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
    vec_t		dot;
    vec_t		dist;
    vec_t		lscale, rscale, scale;
    vec3_t		source_vec;

	if (cls.state != ca_active)
	{
		*left_vol = *right_vol = 255;
		return;
	}

// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels
	
	dot = DotProduct(listener_axis[1], source_vec);

	if (dma.channels == 1 || !dist_mult)
	{ // no attenuation = no spatialization
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 0.5 * (1.0 - dot);
		lscale = 0.5 * (1.0 + dot);
	}

	// add in distance effect
	scale = (1.0 - dist) * rscale;
	*right_vol = (int) (master_vol * scale);
	if (*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (int) (master_vol * scale);
	if (*left_vol < 0)
		*left_vol = 0;
}

/*
=================
S_Spatialize
=================
*/
void S_Spatialize(channel_t *ch)
{
	vec3_t		origin;

	// anything coming from the view entity will always be full volume
	if (ch->entnum == listener_number)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

	if (ch->fixed_origin)
	{
		VectorCopy (ch->origin, origin);
	}
	else
		CL_GetEntitySoundOrigin (ch->entnum, origin);

	S_SpatializeOrigin (origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
}           


/*
=================
S_AllocPlaysound
=================
*/
playsound_t *S_AllocPlaysound (void)
{
	playsound_t	*ps;

	ps = s_freeplays.next;
	if (ps == &s_freeplays)
		return NULL;		// no free playsounds

	// unlink from freelist
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;
	
	return ps;
}


/*
=================
S_FreePlaysound
=================
*/
void S_FreePlaysound (playsound_t *ps)
{
	// unlink from channel
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;

	// add to free list
	ps->next = s_freeplays.next;
	s_freeplays.next->prev = ps;
	ps->prev = &s_freeplays;
	s_freeplays.next = ps;
}



/*
===============
S_IssuePlaysound

Take the next playsound and begin it on the channel
This is never called directly by S_Play*, but only
by the update loop.
===============
*/
void S_IssuePlaysound (playsound_t *ps)
{
	channel_t	*ch;

	if (s_show->value)
		Com_Printf ("Issue %i\n", ps->begin);
	// pick a channel to play on
	ch = S_PickChannel(ps->entnum, ps->entchannel);
	if (!ch)
	{
		S_FreePlaysound (ps);
		return;
	}

	// spatialize
	if (ps->attenuation == ATTN_STATIC)
		ch->dist_mult = ps->attenuation * 0.001;
	else
		ch->dist_mult = ps->attenuation * 0.0005;
	ch->master_vol = ps->volume;
	ch->entnum = ps->entnum;
	ch->entchannel = ps->entchannel;
	ch->sfx = ps->sfx;
	VectorCopy (ps->origin, ch->origin);
	ch->fixed_origin = ps->fixed_origin;

	S_Spatialize(ch);

	S_LoadSound(ch->sfx);
    ch->startSample = s_paintedtime;

	// free the playsound
	S_FreePlaysound (ps);
}

sfx_t *S_RegisterSexedSound (entity_state_t *ent, char *base)
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
			sfx = S_RegisterSound (sexedFilename);
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
====================
S_StartSound

Validates the parms and ques the sound up
if pos is NULL, the sound will be dynamically sourced from the entity
Entchannel 0 will never override a playing sound
====================
*/
void S_StartSound(vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs)
{
	int			vol;
	playsound_t	*ps, *sort;
	int			start;

	if (!s_soundStarted)
		return;

	if (!sfx)
		return;

	if (sfx->Name[0] == '*')
		sfx = S_RegisterSexedSound(&cl_entities[entnum].current, sfx->Name);

	// make sure the sound is loaded
	if (!S_LoadSound (sfx))
		return;		// couldn't load the sound's data

	vol = fvol*255;

	// make the playsound_t
	ps = S_AllocPlaysound ();
	if (!ps)
		return;

	if (origin)
	{
		VectorCopy (origin, ps->origin);
		ps->fixed_origin = true;
	}
	else
		ps->fixed_origin = false;

	ps->entnum = entnum;
	ps->entchannel = entchannel;
	ps->attenuation = attenuation;
	ps->volume = vol;
	ps->sfx = sfx;

	// drift s_beginofs
	start = cl.frame.servertime * 0.001 * dma.speed + s_beginofs;
	if (start < s_paintedtime)
	{
		start = s_paintedtime;
		s_beginofs = start - (cl.frame.servertime * 0.001 * dma.speed);
	}
	else if (start > s_paintedtime + 0.3 * dma.speed)
	{
		start = s_paintedtime + 0.1 * dma.speed;
		s_beginofs = start - (cl.frame.servertime * 0.001 * dma.speed);
	}
	else
	{
		s_beginofs-=10;
	}

	if (!timeofs)
		ps->begin = s_paintedtime;
	else
		ps->begin = start + timeofs * dma.speed;

	// sort into the pending sound list
	for (sort = s_pendingplays.next ; 
		sort != &s_pendingplays && sort->begin < ps->begin ;
		sort = sort->next)
			;

	ps->next = sort;
	ps->prev = sort->prev;

	ps->next->prev = ps;
	ps->prev->next = ps;
}


/*
==================
S_StartLocalSound
==================
*/
void S_StartLocalSound (char *sound)
{
	sfx_t	*sfx;

	if (!s_soundStarted)
		return;
		
	sfx = S_RegisterSound (sound);
	if (!sfx)
	{
		Com_Printf ("S_StartLocalSound: can't cache %s\n", sound);
		return;
	}
	S_StartSound (NULL, listener_number, 0, sfx, 1, 1, 0);
}


/*
==================
S_ClearSoundBuffer
==================
*/
void S_ClearSoundBuffer (void)
{
	int		clear;
		
	if (!s_soundStarted)
		return;

	s_rawend = 0;

	if (dma.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	SNDDMA_BeginPainting ();
	if (dma.buffer)
		Com_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
	SNDDMA_Submit ();
}

/*
==================
S_StopAllSounds
==================
*/
void S_StopAllSounds(void)
{
	int		i;

	if (!s_soundStarted)
		return;

	// clear all the playsounds
	Com_Memset(s_playsounds, 0, sizeof(s_playsounds));
	s_freeplays.next = s_freeplays.prev = &s_freeplays;
	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

	for (i=0 ; i<MAX_PLAYSOUNDS ; i++)
	{
		s_playsounds[i].prev = &s_freeplays;
		s_playsounds[i].next = s_freeplays.next;
		s_playsounds[i].prev->next = &s_playsounds[i];
		s_playsounds[i].next->prev = &s_playsounds[i];
	}

	// clear all the channels
	Com_Memset(s_channels, 0, sizeof(s_channels));
	Com_Memset(loop_channels, 0, sizeof(loop_channels));

	S_ClearSoundBuffer ();
}

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

		sfx = cl.sound_precache[sounds[i]];
		if (!sfx)
			continue;		// bad sound effect
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
S_RawSamples

Cinematic streaming and voice over network
============
*/
void S_RawSamples (int samples, int rate, int width, int channels, byte *data)
{
	int		i;
	int		src, dst;
	float	scale;

	if (!s_soundStarted)
		return;

	if (s_rawend < s_paintedtime)
		s_rawend = s_paintedtime;
	scale = (float)rate / dma.speed;

//Com_Printf ("%i < %i < %i\n", s_soundtime, s_paintedtime, s_rawend);
	if (channels == 2 && width == 2)
	{
		if (scale == 1.0)
		{	// optimized case
			for (i=0 ; i<samples ; i++)
			{
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left =
				    LittleShort(((short *)data)[i*2]) << 8;
				s_rawsamples[dst].right =
				    LittleShort(((short *)data)[i*2+1]) << 8;
			}
		}
		else
		{
			for (i=0 ; ; i++)
			{
				src = i*scale;
				if (src >= samples)
					break;
				dst = s_rawend&(MAX_RAW_SAMPLES-1);
				s_rawend++;
				s_rawsamples[dst].left =
				    LittleShort(((short *)data)[src*2]) << 8;
				s_rawsamples[dst].right =
				    LittleShort(((short *)data)[src*2+1]) << 8;
			}
		}
	}
	else if (channels == 1 && width == 2)
	{
		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend&(MAX_RAW_SAMPLES-1);
			s_rawend++;
			s_rawsamples[dst].left =
			    LittleShort(((short *)data)[src]) << 8;
			s_rawsamples[dst].right =
			    LittleShort(((short *)data)[src]) << 8;
		}
	}
	else if (channels == 2 && width == 1)
	{
		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend&(MAX_RAW_SAMPLES-1);
			s_rawend++;
			s_rawsamples[dst].left =
			    ((char *)data)[src*2] << 16;
			s_rawsamples[dst].right =
			    ((char *)data)[src*2+1] << 16;
		}
	}
	else if (channels == 1 && width == 1)
	{
		for (i=0 ; ; i++)
		{
			src = i*scale;
			if (src >= samples)
				break;
			dst = s_rawend&(MAX_RAW_SAMPLES-1);
			s_rawend++;
			s_rawsamples[dst].left =
			    (((byte *)data)[src]-128) << 16;
			s_rawsamples[dst].right = (((byte *)data)[src]-128) << 16;
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

// mix some sound
	S_Update_();
}

void GetSoundtime(void)
{
	int		samplepos;
	static	int		buffers;
	static	int		oldsamplepos;
	int		fullsamples;
	
	fullsamples = dma.samples / dma.channels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
	samplepos = SNDDMA_GetDMAPos();

	if (samplepos < oldsamplepos)
	{
		buffers++;					// buffer wrapped
		
		if (s_paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			buffers = 0;
			s_paintedtime = fullsamples;
			S_StopAllSounds ();
		}
	}
	oldsamplepos = samplepos;

	s_soundtime = buffers*fullsamples + samplepos/dma.channels;
}


void S_Update_(void)
{
	unsigned        endtime;
	int				samps;

	if (!s_soundStarted)
		return;

	SNDDMA_BeginPainting ();

	if (!dma.buffer)
		return;

// Updates DMA time
	GetSoundtime();

// check to make sure that we haven't overshot
	if (s_paintedtime < s_soundtime)
	{
		Com_DPrintf ("S_Update_ : overflow\n");
		s_paintedtime = s_soundtime;
	}

// mix ahead of current position
	endtime = s_soundtime + s_mixahead->value * dma.speed;
//endtime = (s_soundtime + 4096) & ~4095;

	// mix to an even submission block size
	endtime = (endtime + dma.submission_chunk-1)
		& ~(dma.submission_chunk-1);
	samps = dma.samples >> (dma.channels-1);
	if (endtime - s_soundtime > samps)
		endtime = s_soundtime + samps;

	S_PaintChannels (endtime);

	SNDDMA_Submit ();
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
	sfx_t	*sfx;
	
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

