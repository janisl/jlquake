// cl_main.c  -- client main loop

/*
 * $Header: /H2 Mission Pack/CL_MAIN.C 12    3/16/98 5:32p Jweier $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

// these two are not intended to be set directly
Cvar*	cl_name;
Cvar*	cl_color;
Cvar*	cl_playerclass;

Cvar*	cl_shownet;
Cvar*	cl_nolerp;

Cvar*	lookspring;
Cvar*	lookstrafe;
Cvar*	sensitivity;
static float save_sensitivity;

Cvar*	m_pitch;
Cvar*	m_yaw;
Cvar*	m_forward;
Cvar*	m_side;

Cvar	*cl_lightlevel;


client_static_t	cls;
client_state_t	cl;
// FIXME: put these on hunk?
entity_t		cl_entities[MAX_EDICTS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];

image_t*		gl_extra_textures[MAX_EXTRA_TEXTURES];   // generic textures for models

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	cls.message.Clear();

// clear other arrays	
	Com_Memset(cl_entities, 0, sizeof(cl_entities));
	CL_ClearDlights();
	CL_ClearLightStyles();
	CL_ClearTEnts();
	CL_ClearEffects();

	cl.current_frame = cl.current_sequence = 99;
	cl.reference_frame = cl.last_frame = cl.last_sequence = 199;
	cl.need_build = 2;

	plaquemessage = "";

	SB_InvReset();
}

void CL_RemoveGIPFiles(const char *path)
{
	if (!fs_homepath)
	{
		return;
	}
	char* netpath;
	if (path)
	{
		netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, path);
	}
	else
	{
		netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, "");
		netpath[String::Length(netpath) - 1] = 0;
	}
	int numSysFiles;
	char** sysFiles = Sys_ListFiles(netpath, ".gip", NULL, &numSysFiles, false);
	for (int i = 0 ; i < numSysFiles; i++)
	{
		if (path)
		{
			netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s/%s", path, sysFiles[i]));
		}
		else
		{
			netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, sysFiles[i]);
		}
		FS_Remove(netpath);
	}
	Sys_FreeFileList(sysFiles);
}

void CL_CopyFiles(const char* source, const char* ext, const char* dest)
{
	char* netpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, source);
	if (!source[0])
	{
		netpath[String::Length(netpath) - 1] = 0;
	}
	int numSysFiles;
	char** sysFiles = Sys_ListFiles(netpath, ext, NULL, &numSysFiles, false);
	for (int i = 0 ; i < numSysFiles; i++)
	{
		char* srcpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s%s", source, sysFiles[i]));
		char* dstpath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, va("%s%s", dest, sysFiles[i]));
		FS_CopyFile(srcpath, dstpath);
	}
	Sys_FreeFileList(sysFiles);
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	CL_ClearParticles ();	//jfm: need to clear parts because some now check world
	S_StopAllSounds();// stop sounds (especially looping!)
	loading_stage = 0;

// bring the console down and fade the colors back to normal
//	SCR_BringDownConsole ();

// if running a local server, shut it down
	if (cls.demoplayback)
		CL_StopPlayback ();
	else if (cls.state == ca_connected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		Con_DPrintf ("Sending clc_disconnect\n");
		cls.message.Clear();
		cls.message.WriteByte(clc_disconnect);
		NET_SendUnreliableMessage (cls.netcon, &cls.message);
		cls.message.Clear();
		NET_Close (cls.netcon);

		cls.state = ca_disconnected;
		if (sv.active)
			Host_ShutdownServer(false);

		CL_RemoveGIPFiles(NULL);
	}

	cls.demoplayback = cls.timedemo = false;
	cls.signon = 0;
}

void CL_Disconnect_f (void)
{
	CL_Disconnect ();
	if (sv.active)
		Host_ShutdownServer (false);
}




/*
=====================
CL_EstablishConnection

Host should be either "local" or a net address to be passed on
=====================
*/
void CL_EstablishConnection (const char *host)
{
	if (cls.state == ca_dedicated)
		return;

	if (cls.demoplayback)
		return;

	CL_Disconnect ();

	cls.netcon = NET_Connect (host);
	if (!cls.netcon)
		Host_Error ("CL_Connect: connect failed\n");
	Con_DPrintf ("CL_EstablishConnection: connected to %s\n", host);
	
	cls.demonum = -1;			// not in the demo loop now
	cls.state = ca_connected;
	cls.signon = 0;				// need all the signon messages before playing
}

/*
=====================
CL_SignonReply

An svc_signonnum has been received, perform a client side setup
=====================
*/
void CL_SignonReply (void)
{
	char 	str[8192];

Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		cls.message.WriteByte(clc_stringcmd);
		cls.message.WriteString2("prespawn");
		break;
		
	case 2:		
		cls.message.WriteByte(clc_stringcmd);
		cls.message.WriteString2(va("name \"%s\"\n", cl_name->string));
	
		cls.message.WriteByte(clc_stringcmd);
		cls.message.WriteString2(va("playerclass %i\n", (int)cl_playerclass->value));

		cls.message.WriteByte(clc_stringcmd);
		cls.message.WriteString2(va("color %i %i\n", ((int)cl_color->value)>>4, ((int)cl_color->value)&15));

		cls.message.WriteByte(clc_stringcmd);
		sprintf (str, "spawn %s", cls.spawnparms);
		cls.message.WriteString2(str);
		break;
		
	case 3:	
		cls.message.WriteByte(clc_stringcmd);
		cls.message.WriteString2("begin");
		break;
		
	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	SCR_BeginLoadingPlaque ();

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str);
	cls.demonum++;
}

/*
==============
CL_PrintEntities_f
==============
*/
void CL_PrintEntities_f (void)
{
	entity_t	*ent;
	int			i;
	
	for (i=0,ent=cl_entities ; i<cl.num_entities ; i++,ent++)
	{
		Con_Printf ("%3i:",i);
		if (!ent->state.modelindex)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		,R_ModelName(cl.model_precache[ent->state.modelindex]),ent->state.frame, ent->state.origin[0], ent->state.origin[1], ent->state.origin[2], ent->state.angles[0], ent->state.angles[1], ent->state.angles[2]);
	}
}

/*
===============
CL_LerpPoint

Determines the fraction between the last two messages that the objects
should be put at.
===============
*/
float	CL_LerpPoint (void)
{
	float	f, frac;

	f = cl.mtime[0] - cl.mtime[1];
	
	if (!f || cl_nolerp->value || cls.timedemo || sv.active)
	{
		cl.serverTimeFloat = cl.mtime[0];
		cl.serverTime = (int)(cl.serverTimeFloat * 1000);
		return 1;
	}
		
	if (f > 0.1)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}
	frac = (cl.serverTimeFloat - cl.mtime[1]) / f;
//Con_Printf ("frac: %f\n",frac);
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			cl.serverTimeFloat = cl.mtime[1];
			cl.serverTime = (int)(cl.serverTimeFloat * 1000);
//				Con_Printf ("low frac\n");
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			cl.serverTimeFloat = cl.mtime[0];
			cl.serverTime = (int)(cl.serverTimeFloat * 1000);
//				Con_Printf ("high frac\n");
		}
		frac = 1;
	}
		
	return frac;
}

void R_HandleCustomSkin(refEntity_t* Ent, int PlayerNum)
{
	if (Ent->skinNum >= 100)
	{
		if (Ent->skinNum > 255) 
		{
			Sys_Error ("skinnum > 255");
		}

		if (!gl_extra_textures[Ent->skinNum - 100])  // Need to load it in
		{
			char temp[40];
			String::Sprintf(temp, sizeof(temp), "gfx/skin%d.lmp", Ent->skinNum);
			gl_extra_textures[Ent->skinNum - 100] = R_CachePic(temp);
		}

		Ent->customSkin = R_GetImageHandle(gl_extra_textures[Ent->skinNum - 100]);
	}
	else if (PlayerNum >= 0 && Ent->hModel)
	{
		// we can't dynamically colormap textures, so they are cached
		// seperately for the players.  Heads are just uncolored.
		if (Ent->hModel == player_models[0] ||
			Ent->hModel == player_models[1] ||
			Ent->hModel == player_models[2] ||
			Ent->hModel == player_models[3] ||
			Ent->hModel == player_models[4])
		{
			Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
		}
	}
}

/*
===============
CL_RelinkEntities
===============
*/
void CL_RelinkEntities (void)
{
	entity_t	*ent;
	int			i, j;
	float		frac, f, d;
	vec3_t		delta;
	//float		bobjrotate;
	vec3_t		oldorg;
	int c;

	c = 0;
// determine partial update time	
	frac = CL_LerpPoint ();

	R_ClearScene();

//
// interpolate player info
//
	for (i=0 ; i<3 ; i++)
		cl.velocity[i] = cl.mvelocity[1][i] + 
			frac * (cl.mvelocity[0][i] - cl.mvelocity[1][i]);

	if (cls.demoplayback && !intro_playing)
	{
	// interpolate the angles	
		for (j=0 ; j<3 ; j++)
		{
			d = cl.mviewangles[0][j] - cl.mviewangles[1][j];
			if (d > 180)
				d -= 360;
			else if (d < -180)
				d += 360;
			cl.viewangles[j] = cl.mviewangles[1][j] + frac*d;
		}
	}
	
	//bobjrotate = AngleMod(100*(cl.time+ent->origin[0]+ent->origin[1]));
	
// start on the entity after the world
	for (i=1,ent=cl_entities+1 ; i<cl.num_entities ; i++,ent++)
	{
		if (!ent->state.modelindex)
		{
			// empty slot
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0] && !(ent->baseline.flags & BE_ON))
		{
			ent->state.modelindex = 0;
			continue;
		}

		VectorCopy (ent->state.origin, oldorg);

		if (ent->msgtime != cl.mtime[0])
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->state.origin);
			VectorCopy (ent->msg_angles[0], ent->state.angles);
		}
		else
		{	// if the delta is large, assume a teleport and don't lerp
			f = frac;
			for (j=0 ; j<3 ; j++)
			{
				delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];

				if (delta[j] > 100 || delta[j] < -100)
					f = 1;		// assume a teleportation, not a motion
			}

		// interpolate the origin and angles
			for (j=0 ; j<3 ; j++)
			{
				ent->state.origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->state.angles[j] = ent->msg_angles[1][j] + f*d;
			}
			
		}

// rotate binary objects locally
// BG: Moved to r_alias.c / gl_rmain.c
		//if(ent->model->flags&H2MDLEF_ROTATE)
		//{
		//	ent->angles[1] = AngleMod(ent->origin[0]+ent->origin[1]
		//		+(100*cl.time));
		//}


		c++;
		if (ent->state.effects & EF_DARKFIELD)
			CLH2_DarkFieldParticles (ent->state.origin);
		if (ent->state.effects & EF_MUZZLEFLASH)
		{
			if (cl_prettylights->value)
			{
				CLH2_MuzzleFlashLight(i, ent->state.origin, ent->state.angles, true);
			}
		}
		if (ent->state.effects & EF_BRIGHTLIGHT)
		{			
			if (cl_prettylights->value)
			{
				CLH2_BrightLight(i, ent->state.origin);
			}
		}
		if (ent->state.effects & EF_DIMLIGHT)
		{			
			if (cl_prettylights->value)
			{
				CLH2_DimLight(i, ent->state.origin);
			}
		}
		if (ent->state.effects & EF_DARKLIGHT)
		{			
			if (cl_prettylights->value)
			{
				CLH2_DarkLight(i, ent->state.origin);
			}
		}
		if (ent->state.effects & EF_LIGHT)
		{			
			if (cl_prettylights->value)
			{
				CLH2_Light(i, ent->state.origin);
			}
		}

		int ModelFlags = R_ModelFlags(cl.model_precache[ent->state.modelindex]);
		if (ModelFlags & H2MDLEF_GIB)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_blood);
		else if (ModelFlags & H2MDLEF_ZOMGIB)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_slight_blood);
		else if (ModelFlags & H2MDLEF_BLOODSHOT)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_bloodshot);
		else if (ModelFlags & H2MDLEF_TRACER)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_tracer);
		else if (ModelFlags & H2MDLEF_TRACER2)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_tracer2);
		else if (ModelFlags & H2MDLEF_ROCKET)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_rocket_trail);
		}
		else if (ModelFlags & H2MDLEF_FIREBALL)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_fireball);
			if (cl_prettylights->value)
			{
				CLH2_FireBallLight(i, ent->state.origin);
			}
		}
		else if (ModelFlags & H2MDLEF_ACIDBALL)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_acidball);
			if (cl_prettylights->value)
			{
				CLH2_FireBallLight(i, ent->state.origin);
			}
		}
		else if (ModelFlags & H2MDLEF_ICE)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_ice);
		}
		else if (ModelFlags & H2MDLEF_SPIT)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_spit);
			if (cl_prettylights->value)
			{
				CLH2_SpitLight(i, ent->state.origin);
			}
		}
		else if (ModelFlags & H2MDLEF_SPELL)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_spell);
		}
		else if (ModelFlags & H2MDLEF_GRENADE)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_smoke);
		else if (ModelFlags & H2MDLEF_TRACER3)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_voor_trail);
		else if (ModelFlags & H2MDLEF_VORP_MISSILE)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_vorpal);
		}
		else if (ModelFlags & H2MDLEF_SET_STAFF)
		{
			CLH2_TrailParticles (oldorg, ent->state.origin,rt_setstaff);
		}
		else if (ModelFlags & H2MDLEF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
				CLH2_TrailParticles (oldorg, ent->state.origin, rt_magicmissile);
		}
		else if (ModelFlags & H2MDLEF_BONESHARD)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_boneshard);
		else if (ModelFlags & H2MDLEF_SCARAB)
			CLH2_TrailParticles (oldorg, ent->state.origin, rt_scarab);

		if ( ent->state.effects & EF_NODRAW )
			continue;

		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(ent->state.origin, rent.origin);
		rent.hModel = cl.model_precache[ent->state.modelindex];
		rent.frame = ent->state.frame;
		rent.shaderTime = ent->syncbase;
		rent.skinNum = ent->state.skinnum;
		CL_SetRefEntAxis(&rent, ent->state.angles, ent->state.scale, ent->state.colormap, ent->state.abslight, ent->state.drawflags);
		R_HandleCustomSkin(&rent, i <= cl.maxclients ? i - 1 : -1);
		if (i == cl.viewentity && !chase_active->value)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntityToScene(&rent);
	}

/*	if (c != lastc)
	{
		Con_Printf("Number of entities: %d\n",c);
		lastc = c;
	}*/
}

static void CL_LinkStaticEntities()
{
	entity_t* pent = cl_static_entities;
	for (int i = 0; i < cl.num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_precache[pent->state.modelindex];
		rent.frame = pent->state.frame;
		rent.shaderTime = pent->syncbase;
		rent.skinNum = pent->state.skinnum;
		CL_SetRefEntAxis(&rent, pent->state.angles, pent->state.scale, pent->state.colormap, pent->state.abslight, pent->state.drawflags);
		R_HandleCustomSkin(&rent, -1);
		R_AddRefEntityToScene(&rent);
	}
}

/*
===============
CL_ReadFromServer

Read all incoming data from the server
===============
*/
int CL_ReadFromServer (void)
{
	int		ret;

	cl.oldtime = cl.serverTimeFloat;
	cl.serverTimeFloat += host_frametime;
	cl.serverTime = (int)(cl.serverTimeFloat * 1000);
	
	do
	{
		ret = CL_GetMessage ();
		if (ret == -1)
			Host_Error ("CL_ReadFromServer: lost server connection");
		if (!ret)
			break;
		
		cl.last_received_message = realtime;
		CL_ParseServerMessage ();
	} while (ret && cls.state == ca_connected);
	
	if (cl_shownet->value)
		Con_Printf ("\n");

	CL_RelinkEntities ();
	CL_UpdateTEnts ();
	CL_LinkStaticEntities();

//
// bring the links up to date
//
	return 0;
}

/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	usercmd_t		cmd;

	if (cls.state != ca_connected)
		return;

	if (cls.signon == SIGNONS)
	{
	// get basic movement from keyboard
		CL_BaseMove (&cmd);
	
	// allow mice or other external controllers to add to the move
		CL_MouseMove(&cmd);
	
	// send the unreliable message
		CL_SendMove (&cmd);
	
	}

	if (cls.demoplayback)
	{
		cls.message.Clear();
		return;
	}
	
// send the reliable message
	if (!cls.message.cursize)
		return;		// no message at all
	
	if (!NET_CanSendMessage (cls.netcon))
	{
		Con_DPrintf ("CL_WriteToServer: can't send\n");
		return;
	}

	if (NET_SendMessage (cls.netcon, &cls.message) == -1)
		Host_Error ("CL_WriteToServer: lost server connection");

	cls.message.Clear();
}

void CL_Sensitivity_save_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("sensitivity_save <save/restore>\n");
		return;
	}

	if (String::ICmp(Cmd_Argv(1),"save") == 0)
		save_sensitivity = sensitivity->value;
	else if (String::ICmp(Cmd_Argv(1),"restore") == 0)
		Cvar_SetValue ("sensitivity", save_sensitivity);
}
/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	cls_common = &cls;
	cl_common = &cl;
	cls.message.InitOOB(cls.message_buf, sizeof(cls.message_buf));

	CL_SharedInit();

	CL_InitInput ();

	
//
// register our commands
//
	cl_name = Cvar_Get("_cl_name", "player", CVAR_ARCHIVE);
	cl_color = Cvar_Get("_cl_color", "0", CVAR_ARCHIVE);
	cl_playerclass = Cvar_Get("_cl_playerclass", "5", CVAR_ARCHIVE);
	cl_upspeed = Cvar_Get("cl_upspeed", "200", 0);
	cl_forwardspeed = Cvar_Get("cl_forwardspeed", "200", CVAR_ARCHIVE);
	cl_backspeed = Cvar_Get("cl_backspeed", "200", CVAR_ARCHIVE);
	cl_sidespeed = Cvar_Get("cl_sidespeed","225", 0);
	cl_movespeedkey = Cvar_Get("cl_movespeedkey", "2.0", 0);
	cl_yawspeed = Cvar_Get("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get("cl_anglespeedkey", "1.5", 0);
	cl_shownet = Cvar_Get("cl_shownet", "0", 0);	// can be 0, 1, or 2
	cl_nolerp = Cvar_Get("cl_nolerp", "0", 0);
	lookspring = Cvar_Get("lookspring", "0", CVAR_ARCHIVE);
	lookstrafe = Cvar_Get("lookstrafe", "0", CVAR_ARCHIVE);
	sensitivity = Cvar_Get("sensitivity", "3", CVAR_ARCHIVE);

	m_pitch = Cvar_Get("m_pitch", "0.022", CVAR_ARCHIVE);
	m_yaw = Cvar_Get("m_yaw", "0.022", CVAR_ARCHIVE);
	m_forward = Cvar_Get("m_forward", "1", CVAR_ARCHIVE);
	m_side = Cvar_Get("m_side", "0.8", CVAR_ARCHIVE);
	cl_prettylights = Cvar_Get("cl_prettylights", "1", 0);

	cl_lightlevel = Cvar_Get ("r_lightlevel", "0", 0);

	Cmd_AddCommand ("entities", CL_PrintEntities_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);
	Cmd_AddCommand ("sensitivity_save", CL_Sensitivity_save_f);
}

void CL_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles, int scale, int colorshade, int abslight, int drawflags)
{
	if (drawflags & DRF_TRANSLUCENT)
	{
		ent->renderfx |= RF_WATERTRANS;
	}

	vec3_t angles;
	if (R_IsMeshModel(ent->hModel))
	{
		if (R_ModelFlags(ent->hModel) & H2MDLEF_FACE_VIEW)
		{
			//	yaw and pitch must be 0 so that renderer can safely multply matrices.
			angles[PITCH] = 0;
			angles[YAW] = 0;
			angles[ROLL] = ent_angles[ROLL];
		}
		else 
		{
			if (R_ModelFlags(ent->hModel) & H2MDLEF_ROTATE)
			{
				angles[YAW] = AngleMod((ent->origin[0] + ent->origin[1]) * 0.8 + (108 * cl.serverTimeFloat));
			}
			else
			{
				angles[YAW] = ent_angles[YAW];
			}
			angles[ROLL] = ent_angles[ROLL];
			// stupid quake bug
			angles[PITCH] = -ent_angles[PITCH];
		}

		AnglesToAxis(angles, ent->axis);

		if ((R_ModelFlags(ent->hModel) & H2MDLEF_ROTATE) || (scale != 0 && scale != 100))
		{
			ent->renderfx |= RF_LIGHTING_ORIGIN;
			VectorCopy(ent->origin, ent->lightingOrigin);
		}

		if (R_ModelFlags(ent->hModel) & H2MDLEF_ROTATE)
		{
			// Floating motion
			float delta = sin(ent->origin[0] + ent->origin[1] + (cl.serverTimeFloat * 3)) * 5.5;
			VectorMA(ent->origin, delta, ent->axis[2], ent->origin);
			abslight = 60 + 34 + sin(ent->origin[0] + ent->origin[1] + (cl.serverTimeFloat * 3.8)) * 34;
			drawflags |= MLS_ABSLIGHT;
		}

		if (scale != 0 && scale != 100)
		{
			float entScale = (float)scale / 100.0;
			float esx;
			float esy;
			float esz;
			switch (drawflags & SCALE_TYPE_MASKIN)
			{
			case SCALE_TYPE_UNIFORM:
				esx = entScale;
				esy = entScale;
				esz = entScale;
				break;
			case SCALE_TYPE_XYONLY:
				esx = entScale;
				esy = entScale;
				esz = 1;
				break;
			case SCALE_TYPE_ZONLY:
				esx = 1;
				esy = 1;
				esz = entScale;
				break;
			}
			float etz;
			switch (drawflags & SCALE_ORIGIN_MASKIN)
			{
			case SCALE_ORIGIN_CENTER:
				etz = 0.5;
				break;
			case SCALE_ORIGIN_BOTTOM:
				etz = 0;
				break;
			case SCALE_ORIGIN_TOP:
				etz = 1.0;
				break;
			}

			vec3_t Out;
			R_CalculateModelScaleOffset(ent->hModel, esx, esy, esz, etz, Out);
			VectorMA(ent->origin, Out[0], ent->axis[0], ent->origin);
			VectorMA(ent->origin, Out[1], ent->axis[1], ent->origin);
			VectorMA(ent->origin, Out[2], ent->axis[2], ent->origin);
			VectorScale(ent->axis[0], esx, ent->axis[0]);
			VectorScale(ent->axis[1], esy, ent->axis[1]);
			VectorScale(ent->axis[2], esz, ent->axis[2]);
			ent->nonNormalizedAxes = true;
		}
	}
	else
	{
		angles[YAW] = ent_angles[YAW];
		angles[ROLL] = ent_angles[ROLL];
		angles[PITCH] = ent_angles[PITCH];

		AnglesToAxis(angles, ent->axis);
	}

	if (colorshade)
	{
		ent->renderfx |= RF_COLORSHADE;
		ent->shaderRGBA[0] = (int)(RTint[colorshade] * 255);
		ent->shaderRGBA[1] = (int)(GTint[colorshade] * 255);
		ent->shaderRGBA[2] = (int)(BTint[colorshade] * 255);
	}

	int mls = drawflags & MLS_MASKIN;
	if (mls == MLS_ABSLIGHT)
	{
		ent->renderfx |= RF_ABSOLUTE_LIGHT;
		ent->radius = abslight / 256.0;
	}
	else if (mls != MLS_NONE)
	{
		// Use a model light style (25-30)
		ent->renderfx |= RF_ABSOLUTE_LIGHT;
		ent->radius = cl_lightstyle[24 + mls].value[0] / 2;
	}
}

void CIN_StartedPlayback()
{
}

bool CIN_IsInCinematicState()
{
	return false;
}

void CIN_FinishCinematic()
{
}

float* CL_GetSimOrg()
{
	return cl_entities[cl.viewentity].state.origin;
}
