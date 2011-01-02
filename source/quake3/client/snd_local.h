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
// snd_local.h -- private sound definations


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../../../libs/client/snd_public.h"
#include "snd_public.h"

#include "../../../libs/client/snd_local.h"

#define START_SAMPLE_IMMEDIATE	0x7fffffff

typedef struct loopSound_s {
	vec3_t		origin;
	vec3_t		velocity;
	sfx_t		*sfx;
	int			mergeFrame;
	qboolean	active;
	qboolean	kill;
	qboolean	doppler;
	float		dopplerScale;
	float		oldDopplerScale;
	int			framenum;
} loopSound_t;



extern	vec3_t	listener_forward;
extern	vec3_t	listener_right;
extern	vec3_t	listener_up;

extern QCvar	*s_nosound;
extern QCvar	*s_khz;
extern QCvar	*s_show;
extern QCvar	*s_mixahead;

extern QCvar	*s_separation;

void S_memoryLoad(sfx_t *sfx);
portable_samplepair_t *S_GetRawSamplePointer();

// spatializes a channel
void S_Spatialize(channel_t *ch);
