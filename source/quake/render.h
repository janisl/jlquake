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

// refresh.h -- public interface to refresh functions

#define	MAXCLIPPLANES	11

#define	TOP_RANGE		16			// soldier uniform colors
#define	BOTTOM_RANGE	96

//	Model effects
#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail

//=============================================================================

struct entity_t
{
	qboolean				forcelink;		// model changed

	entity_state_t			baseline;		// to fill in defaults in updates

	double					msgtime;		// time of last update
	vec3_t					msg_origins[2];	// last two updates (0 is newest)	
	vec3_t					origin;
	vec3_t					msg_angles[2];	// last two updates (0 is newest)
	vec3_t					angles;	
	qhandle_t				model;			// NULL = no model
	int						frame;
	float					syncbase;		// for client-side animations
	int						colormap;
	int						effects;		// light, particals, etc
	int						skinnum;		// for Alias models
};


//
// refresh
//
extern	int		reinit_surfcache;


extern	refdef_t	r_refdef;

void R_Init (void);
void R_InitTextures (void);
void R_InitEfrags (void);
void R_RenderView (void);		// must set r_refdef first
void R_ViewChanged (vrect_t *pvrect, int lineadj, float aspect);
								// called whenever r_refdef or vid change

void R_NewMap (void);


void R_ParseParticleEffect (void);
void R_RunParticleEffect (const vec3_t org, const vec3_t dir, int color, int count);
void R_RocketTrail (vec3_t start, vec3_t end, int type);

void R_EntityParticles (entity_t *ent);
void R_BlobExplosion (vec3_t org);
void R_ParticleExplosion (vec3_t org);
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength);
void R_LavaSplash (vec3_t org);
void R_TeleportSplash (vec3_t org);

void R_PushDlights (void);


//
// surface cache related
//
extern	int		reinit_surfcache;	// if 1, surface cache is currently empty and
extern qboolean	r_cache_thrash;	// set if thrashing the surface cache

int	D_SurfaceCacheForRes (int width, int height);
void D_DeleteSurfaceCache (void);
void D_InitCaches (void *buffer, int size);
void R_SetVrect (vrect_t *pvrect, vrect_t *pvrectin, int lineadj);

void R_HandleRefEntColormap(refEntity_t* Ent, int ColorMap);
void R_ClearScene();
void R_AddRefEntToScene(refEntity_t* Ent);
void R_TranslatePlayerSkin (int playernum);

void	Mod_Init (void);
void	Mod_ClearAll (void);
qhandle_t Mod_ForName (char *name, qboolean crash);
int Mod_GetNumFrames(qhandle_t Handle);
int Mod_GetFlags(qhandle_t Handle);
void Mod_PrintFrameName(qhandle_t m, int frame);
bool Mod_IsAliasModel(qhandle_t Handle);
const char* Mod_GetName(qhandle_t Handle);
int Mod_GetSyncType(qhandle_t Handle);

extern	QCvar*	gl_doubleeyes;
