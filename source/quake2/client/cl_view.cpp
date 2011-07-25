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
// cl_view.c -- player rendering positioning

#include "client.h"
#include "../ref_gl/gl_local.h"

//=============
//
// development tools for weapons
//
int			gun_frame;
qhandle_t	gun_model;

//=============

QCvar		*crosshair;
QCvar		*cl_testparticles;
QCvar		*cl_testentities;
QCvar		*cl_testlights;
QCvar		*cl_testblend;

QCvar		*cl_stats;

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void V_TestParticles (void)
{
	particle_t	*p;
	int			i, j;
	float		d, r, u;

	r_numparticles = MAX_PARTICLES;
	for (i=0 ; i<r_numparticles ; i++)
	{
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);
		p = &backEndData[tr.smpFrame]->particles[i];

		for (j=0 ; j<3 ; j++)
			p->origin[j] = cl.refdef.vieworg[j] + cl.refdef.viewaxis[0][j]*d -
			cl.refdef.viewaxis[1][j]*r + cl.refdef.viewaxis[2][j]*u;

		p->rgba[0] = r_palette[8][0];
		p->rgba[1] = r_palette[8][1];
		p->rgba[2] = r_palette[8][2];
		p->rgba[3] = (int)(cl_testparticles->value * 255);
		p->size = 1;
	}
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
void V_TestEntities (void)
{
	int			i, j;
	float		f, r;
	trRefEntity_t	*ent;

	r_numentities = 32;
	r_firstSceneEntity = 0;
	Com_Memset(backEndData[tr.smpFrame]->entities, 0, sizeof(backEndData[tr.smpFrame]->entities));

	for (i=0 ; i<r_numentities ; i++)
	{
		ent = &backEndData[tr.smpFrame]->entities[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			ent->e.origin[j] = cl.refdef.vieworg[j] + cl.refdef.viewaxis[0][j]*f -
			cl.refdef.viewaxis[1][j]*r;
		AxisClear(ent->e.axis);

		ent->e.hModel = cl.baseclientinfo.model;
		ent->e.customSkin = R_GetImageHandle(cl.baseclientinfo.skin);
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	int			i, j;
	float		f, r;
	dlight_t	*dl;

	r_numdlights = 32;
	Com_Memset(backEndData[tr.smpFrame]->dlights, 0, sizeof(backEndData[tr.smpFrame]->dlights));

	for (i=0 ; i<r_numdlights ; i++)
	{
		dl = &backEndData[tr.smpFrame]->dlights[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.refdef.viewaxis[0][j]*f -
			cl.refdef.viewaxis[1][j]*r;
		dl->color[0] = ((i%6)+1) & 1;
		dl->color[1] = (((i%6)+1) & 2)>>1;
		dl->color[2] = (((i%6)+1) & 4)>>2;
		dl->radius = 200;
	}
}

//===================================================================

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh (void)
{
	char		mapname[32];
	int			i;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded

	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);

	// let the render dll load the map
	QStr::Cpy(mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[QStr::Length(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf ("Map: %s\r", mapname); 
	SCR_UpdateScreen ();
	R_BeginRegistrationAndLoadWorld (mapname);
	Com_Printf ("                                     \r");

	// precache status bar pics
	Com_Printf ("pics\r"); 
	SCR_UpdateScreen ();
	SCR_TouchPics ();
	Com_Printf ("                                     \r");

	CL_RegisterTEntModels ();

	num_cl_weaponmodels = 1;
	QStr::Cpy(cl_weaponmodels[0], "weapon.md2");

	for (i=1 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
	{
		QStr::Cpy(name, cl.configstrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
			Com_Printf ("%s\r", name); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		IN_ProcessEvents();
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				QStr::NCpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		} 
		else
		{
			cl.model_draw[i] = R_RegisterModel(cl.configstrings[CS_MODELS+i]);
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel(QStr::Atoi(cl.configstrings[CS_MODELS + i] + 1));
			else
				cl.model_clip[i] = 0;
		}
		if (name[0] != '*')
			Com_Printf ("                                     \r");
	}

	Com_Printf ("images\r", i); 
	SCR_UpdateScreen ();
	for (i=1 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
	{
		cl.image_precache[i] = R_RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
		IN_ProcessEvents();
	}
	
	Com_Printf ("                                     \r");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_Printf ("client %i\r", i); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		IN_ProcessEvents();
		CL_ParseClientinfo (i);
		Com_Printf ("                                     \r");
	}

	CL_LoadClientinfo (&cl.baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	Com_Printf ("sky\r", i); 
	SCR_UpdateScreen ();
	rotate = QStr::Atof(cl.configstrings[CS_SKYROTATE]);
	sscanf (cl.configstrings[CS_SKYAXIS], "%f %f %f", 
		&axis[0], &axis[1], &axis[2]);
	R_SetSky (cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf ("                                     \r");

	// the renderer can now free unneeded stuff
	re.EndRegistration ();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;

	// start the cd track
	CDAudio_Play (QStr::Atoi(cl.configstrings[CS_CDTRACK]), true);
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

// gun frame debugging functions
void V_Gun_Next_f (void)
{
	gun_frame++;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Prev_f (void)
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Model_f (void)
{
	char	name[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		gun_model = 0;
		return;
	}
	QStr::Sprintf (name, sizeof(name), "models/%s/tris.md2", Cmd_Argv(1));
	gun_model = R_RegisterModel(name);
}

//============================================================================


/*
=================
SCR_DrawCrosshair
=================
*/
void SCR_DrawCrosshair (void)
{
	if (!crosshair->value)
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();
	}

	if (!crosshair_pic[0])
		return;

	UI_DrawNamedPic (scr_vrect.x + ((scr_vrect.width - crosshair_width)>>1)
	, scr_vrect.y + ((scr_vrect.height - crosshair_height)>>1), crosshair_pic);
}

/*
==================
V_RenderView

==================
*/
void V_RenderView( float stereo_separation )
{
	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds_ ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if (cl.frame.valid)
	{
		R_ClearScene();

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			v_blend[0] = 1;
			v_blend[1] = 0.5;
			v_blend[2] = 0.25;
			v_blend[3] = 0.5;
		}

		// offset vieworg appropriately if we're doing stereo separation
		if ( stereo_separation != 0 )
		{
			vec3_t tmp;

			VectorScale( cl.refdef.viewaxis[1], -stereo_separation, tmp );
			VectorAdd(cl.refdef.vieworg, tmp, cl.refdef.vieworg);
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time;

		for (int i = 0; i < MAX_MAP_AREA_BYTES; i++)
		{
			cl.refdef.areamask[i] = ~cl.frame.areabits[i];
		}

		if (!cl_add_entities->value)
			r_numentities = r_firstSceneEntity;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_blend->value)
		{
			VectorClear(v_blend);
		}

		cl.refdef.rdflags = 0;
		if (cl.frame.playerstate.rdflags & Q2RDF_NOWORLDMODEL)
		{
			cl.refdef.rdflags |= RDF_NOWORLDMODEL;
		}
		if (cl.frame.playerstate.rdflags & Q2RDF_IRGOGGLES)
		{
			cl.refdef.rdflags |= RDF_IRGOGGLES;
		}
	}

	re.RenderFrame (&cl.refdef);
	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		FS_Printf(log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);


	SCR_AddDirtyPoint (scr_vrect.x, scr_vrect.y);
	SCR_AddDirtyPoint (scr_vrect.x+scr_vrect.width-1,
		scr_vrect.y+scr_vrect.height-1);

	SCR_DrawCrosshair ();
}


/*
=============
V_Viewpos_f
=============
*/
void V_Viewpos_f (void)
{
	Com_Printf ("(%i %i %i) : %i\n", (int)cl.refdef.vieworg[0],
		(int)cl.refdef.vieworg[1], (int)cl.refdef.vieworg[2], 
		(int)(atan2(cl.refdef.viewaxis[0][1], cl.refdef.viewaxis[0][0]) * 180 / M_PI));
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("gun_next", V_Gun_Next_f);
	Cmd_AddCommand ("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand ("gun_model", V_Gun_Model_f);

	Cmd_AddCommand ("viewpos", V_Viewpos_f);

	crosshair = Cvar_Get ("crosshair", "0", CVAR_ARCHIVE);

	cl_testblend = Cvar_Get ("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get ("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get ("cl_testlights", "0", 0);

	cl_stats = Cvar_Get ("cl_stats", "0", 0);
}
