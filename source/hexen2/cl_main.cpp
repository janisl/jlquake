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
QCvar*	cl_name;
QCvar*	cl_color;
QCvar*	cl_playerclass;

QCvar*	cl_shownet;
QCvar*	cl_nolerp;

QCvar*	lookspring;
QCvar*	lookstrafe;
QCvar*	sensitivity;
static float save_sensitivity;

QCvar*	m_pitch;
QCvar*	m_yaw;
QCvar*	m_forward;
QCvar*	m_side;


client_static_t	cls;
client_state_t	cl;
// FIXME: put these on hunk?
entity_t		cl_entities[MAX_EDICTS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	int			i;

	if (!sv.active)
		Host_ClearMemory ();

// wipe the entire cl structure
	Com_Memset(&cl, 0, sizeof(cl));

	cls.message.Clear();

// clear other arrays	
	Com_Memset(cl_entities, 0, sizeof(cl_entities));
	Com_Memset(cl_dlights, 0, sizeof(cl_dlights));
	Com_Memset(cl_lightstyle, 0, sizeof(cl_lightstyle));
	CL_ClearTEnts();
	CL_ClearEffects();

	cl.current_frame = cl.current_sequence = 99;
	cl.reference_frame = cl.last_frame = cl.last_sequence = 199;
	cl.need_build = 2;

	plaquemessage = "";

	SB_InvReset();
}

static qboolean IsGip(const char* name)
{
	int l = QStr::Length(name);
	if (l < 4)
	{
		return false;
	}
	return !QStr::Cmp(name + l - 4, ".gip");
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
		netpath[QStr::Length(netpath) - 1] = 0;
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
		netpath[QStr::Length(netpath) - 1] = 0;
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
	R_ClearParticles ();	//jfm: need to clear parts because some now check world
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
void CL_EstablishConnection (char *host)
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
		Cache_Report ();		// print remaining memory
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
		if (!ent->model)
		{
			Con_Printf ("EMPTY\n");
			continue;
		}
		Con_Printf ("%s:%2i  (%5.1f,%5.1f,%5.1f) [%5.1f %5.1f %5.1f]\n"
		,Mod_GetName(ent->model),ent->frame, ent->origin[0], ent->origin[1], ent->origin[2], ent->angles[0], ent->angles[1], ent->angles[2]);
	}
}

/*
===============
CL_AllocDlight

===============
*/
dlight_t *CL_AllocDlight (int key)
{
	int		i;
	dlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				Com_Memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			Com_Memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	Com_Memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}


/*
===============
CL_DecayLights

===============
*/
void CL_DecayLights (void)
{
	int			i;
	dlight_t	*dl;
	float		time;
	
	time = cl.time - cl.oldtime;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time || !dl->radius)
			continue;
		
		if (dl->radius > 0)
		{
			dl->radius -= time*dl->decay;
			if (dl->radius < 0)
				dl->radius = 0;
		}
		else
		{
			dl->radius += time*dl->decay;
			if (dl->radius > 0)
				dl->radius = 0;
		}
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
		cl.time = cl.mtime[0];
		return 1;
	}
		
	if (f > 0.1)
	{	// dropped packet, or start of demo
		cl.mtime[1] = cl.mtime[0] - 0.1;
		f = 0.1;
	}
	frac = (cl.time - cl.mtime[1]) / f;
//Con_Printf ("frac: %f\n",frac);
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			cl.time = cl.mtime[1];
//				Con_Printf ("low frac\n");
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			cl.time = cl.mtime[0];
//				Con_Printf ("high frac\n");
		}
		frac = 1;
	}
		
	return frac;
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
	dlight_t	*dl;
	int c;
	static int lastc = 0;

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
		if (!ent->model)
		{
			// empty slot
			continue;
		}

// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.mtime[0] && !(ent->baseline.flags & BE_ON))
		{
			ent->model = 0;
			continue;
		}

		VectorCopy (ent->origin, oldorg);

		if (ent->forcelink || ent->msgtime != cl.mtime[0])
		{	// the entity was not updated in the last message
			// so move to the final spot
			VectorCopy (ent->msg_origins[0], ent->origin);
			VectorCopy (ent->msg_angles[0], ent->angles);
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
				ent->origin[j] = ent->msg_origins[1][j] + f*delta[j];

				d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
				if (d > 180)
					d -= 360;
				else if (d < -180)
					d += 360;
				ent->angles[j] = ent->msg_angles[1][j] + f*d;
			}
			
		}

// rotate binary objects locally
// BG: Moved to r_alias.c / gl_rmain.c
		//if(ent->model->flags&EF_ROTATE)
		//{
		//	ent->angles[1] = AngleMod(ent->origin[0]+ent->origin[1]
		//		+(100*cl.time));
		//}


		c++;
//		if (ent->effects & EF_BRIGHTFIELD);
//			R_EntityParticles (ent);
		if (ent->effects & EF_DARKFIELD)
			R_DarkFieldParticles (ent);
		if (ent->effects & EF_MUZZLEFLASH)
		{
			vec3_t		fv, rv, uv;

			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->origin[2] += 16;
				AngleVectors (ent->angles, fv, rv, uv);
				 
				VectorMA (dl->origin, 18, fv, dl->origin);
				dl->radius = 200 + (rand()&31);
				dl->minlight = 32;
				dl->die = cl.time + 0.1;
			}
		}
		if (ent->effects & EF_BRIGHTLIGHT)
		{			
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->origin[2] += 16;
				dl->radius = 400 + (rand()&31);
				dl->die = cl.time + 0.001;
			}
		}
		if (ent->effects & EF_DIMLIGHT)
		{			
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = 200 + (rand()&31);
				dl->die = cl.time + 0.001;
			}
		}
		if (ent->effects & EF_DARKLIGHT)
		{			
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = 200.0 + (rand()&31);
				dl->die = cl.time + 0.001;
				dl->dark = true;
			}
		}
		if (ent->effects & EF_LIGHT)
		{			
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin,  dl->origin);
				dl->radius = 200;
				dl->die = cl.time + 0.001;
			}
		}

		int ModelFlags = Mod_GetFlags(ent->model);
		if (ModelFlags & EF_GIB)
			R_RocketTrail (oldorg, ent->origin, 2);
		else if (ModelFlags & EF_ZOMGIB)
			R_RocketTrail (oldorg, ent->origin, 4);
		else if (ModelFlags & EF_BLOODSHOT)
			R_RocketTrail (oldorg, ent->origin, 17);
		else if (ModelFlags & EF_TRACER)
			R_RocketTrail (oldorg, ent->origin, 3);
		else if (ModelFlags & EF_TRACER2)
			R_RocketTrail (oldorg, ent->origin, 5);
		else if (ModelFlags & EF_ROCKET)
		{
			R_RocketTrail (oldorg, ent->origin, 0);
		}
		else if (ModelFlags & EF_FIREBALL)
		{
			R_RocketTrail (oldorg, ent->origin, rt_fireball);
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 120 - (rand() % 20);
				dl->die = cl.time + 0.01;
			}
		}
		else if (ModelFlags & EF_ACIDBALL)
		{
			R_RocketTrail (oldorg, ent->origin, rt_acidball);
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = 120 - (rand() % 20);
				dl->die = cl.time + 0.01;
			}
		}
		else if (ModelFlags & EF_ICE)
		{
			R_RocketTrail (oldorg, ent->origin, rt_ice);
		}
		else if (ModelFlags & EF_SPIT)
		{
			R_RocketTrail (oldorg, ent->origin, rt_spit);
			if (cl_prettylights->value)
			{
				dl = CL_AllocDlight (i);
				VectorCopy (ent->origin, dl->origin);
				dl->radius = -120 - (rand() % 20);
				dl->die = cl.time + 0.05;
			}
		}
		else if (ModelFlags & EF_SPELL)
		{
			R_RocketTrail (oldorg, ent->origin, rt_spell);
		}
		else if (ModelFlags & EF_GRENADE)
			R_RocketTrail (oldorg, ent->origin, 1);
		else if (ModelFlags & EF_TRACER3)
			R_RocketTrail (oldorg, ent->origin, 6);
		else if (ModelFlags & EF_VORP_MISSILE)
		{
			R_RocketTrail (oldorg, ent->origin, rt_vorpal);
		}
		else if (ModelFlags & EF_SET_STAFF)
		{
			R_RocketTrail (oldorg, ent->origin,rt_setstaff);
		}
		else if (ModelFlags & EF_MAGICMISSILE)
		{
			if ((rand() & 3) < 1)
				R_RocketTrail (oldorg, ent->origin, rt_magicmissile);
		}
		else if (ModelFlags & EF_BONESHARD)
			R_RocketTrail (oldorg, ent->origin, rt_boneshard);
		else if (ModelFlags & EF_SCARAB)
			R_RocketTrail (oldorg, ent->origin, rt_scarab);

		ent->forcelink = false;

		if ( ent->effects & EF_NODRAW )
			continue;

		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(ent->origin, rent.origin);
		rent.hModel = ent->model;
		rent.frame = ent->frame;
		rent.shaderTime = ent->syncbase;
		rent.skinNum = ent->skinnum;
		CL_SetRefEntAxis(&rent, ent->angles, ent->scale, ent->colorshade, ent->abslight, ent->drawflags);
		R_HandleCustomSkin(&rent, i <= cl.maxclients ? i - 1 : -1);
		if (i == cl.viewentity)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntToScene(&rent);
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
		VectorCopy(pent->origin, rent.origin);
		rent.hModel = pent->model;
		rent.frame = pent->frame;
		rent.shaderTime = pent->syncbase;
		rent.skinNum = pent->skinnum;
		CL_SetRefEntAxis(&rent, pent->angles, pent->scale, pent->colorshade, pent->abslight, pent->drawflags);
		R_HandleCustomSkin(&rent, -1);
		R_AddRefEntToScene(&rent);
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

	cl.oldtime = cl.time;
	cl.time += host_frametime;
	
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

	if (QStr::ICmp(Cmd_Argv(1),"save") == 0)
		save_sensitivity = sensitivity->value;
	else if (QStr::ICmp(Cmd_Argv(1),"restore") == 0)
		Cvar_SetValue ("sensitivity", save_sensitivity);
}
/*
=================
CL_Init
=================
*/
void CL_Init (void)
{	
	cls.message.InitOOB(cls.message_buf, sizeof(cls.message_buf));

	CL_SharedInit();

	CL_InitInput ();
	CL_InitTEnts ();

	
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
	if (Mod_IsAliasModel(ent->hModel))
	{
		if (Mod_GetFlags(ent->hModel) & EF_FACE_VIEW)
		{
			//	yaw and pitch must be 0 so that renderer can safely multply matrices.
			angles[PITCH] = 0;
			angles[YAW] = 0;
			angles[ROLL] = ent_angles[ROLL];
		}
		else 
		{
			if (Mod_GetFlags(ent->hModel) & EF_ROTATE)
			{
				angles[YAW] = AngleMod((ent->origin[0] + ent->origin[1]) * 0.8 + (108 * cl.time));
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

		if ((Mod_GetFlags(ent->hModel) & EF_ROTATE) || (scale != 0 && scale != 100))
		{
			ent->renderfx |= RF_LIGHTING_ORIGIN;
			VectorCopy(ent->origin, ent->lightingOrigin);
		}

		if (Mod_GetFlags(ent->hModel) & EF_ROTATE)
		{
			// Floating motion
			float delta = sin(ent->origin[0] + ent->origin[1] + (cl.time * 3)) * 5.5;
			VectorMA(ent->origin, delta, ent->axis[2], ent->origin);
			abslight = 60 + 34 + sin(ent->origin[0] + ent->origin[1] + (cl.time * 3.8)) * 34;
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
			Mod_CalcScaleOffset(ent->hModel, esx, esy, esz, etz, Out);
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
		ent->radius = d_lightstylevalue[24 + mls] / 2 / 256.0;
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

void CL_SendModelChecksum(const char* name, const void* buffer)
{
}
