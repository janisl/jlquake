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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "../core/file_formats/spr.h"

const char *svc_strings[] =
{
	"q1svc_bad",
	"q1svc_nop",
	"q1svc_disconnect",
	"q1svc_updatestat",
	"q1svc_version",		// [long] server version
	"q1svc_setview",		// [short] entity number
	"q1svc_sound",			// <see code>
	"q1svc_time",			// [float] server time
	"q1svc_print",			// [string] null terminated string
	"q1svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"q1svc_setangle",		// [vec3] set the view angle to this absolute value
	
	"q1svc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"q1svc_lightstyle",		// [byte] [string]
	"q1svc_updatename",		// [byte] [string]
	"q1svc_updatefrags",	// [byte] [short]
	"q1svc_clientdata",		// <shortbits + data>
	"q1svc_stopsound",		// <see code>
	"q1svc_updatecolors",	// [byte] [byte]
	"q1svc_particle",		// [vec3] <variable>
	"q1svc_damage",			// [byte] impact [byte] blood [vec3] from
	
	"q1svc_spawnstatic",
	"OBSOLETE svc_spawnbinary",
	"q1svc_spawnbaseline",
	
	"q1svc_temp_entity",		// <variable>
	"q1svc_setpause",
	"q1svc_signonnum",
	"q1svc_centerprint",
	"q1svc_killedmonster",
	"q1svc_foundsecret",
	"q1svc_spawnstaticsound",
	"q1svc_intermission",
	"q1svc_finale",			// [string] music [string] text
	"q1svc_cdtrack",			// [byte] track [byte] looptrack
	"q1svc_sellscreen",
	"q1svc_cutscene"
};

image_t*	playertextures[16];		// up to 16 color translated skins

//=============================================================================

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;  
 	int		i;
	           
    field_mask = net_message.ReadByte(); 

    if (field_mask & SND_VOLUME)
		volume = net_message.ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	
    if (field_mask & SND_ATTENUATION)
		attenuation = net_message.ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	
	channel = net_message.ReadShort ();
	sound_num = net_message.ReadByte ();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS_Q1)
		Host_Error ("CL_ParseStartSoundPacket: ent = %i", ent);
	
	for (i=0 ; i<3 ; i++)
		pos[i] = net_message.ReadCoord ();
 
    S_StartSound(pos, ent, channel, cl.sound_precache[sound_num], volume / 255.0, attenuation);
}       

/*
==================
CL_KeepaliveMessage

When the client is taking a long time to load stuff, send keepalive messages
so the server doesn't disconnect.
==================
*/
void CL_KeepaliveMessage (void)
{
	float	time;
	static float lastmsg;
	int		ret;
	QMsg	old;
	byte		olddata[8192];
	
	if (sv.active)
		return;		// no need if server is local
	if (cls.demoplayback)
		return;

// read messages from server, should just be nops
	old.Copy(olddata, sizeof(olddata), net_message);

	do
	{
		ret = CL_GetMessage ();
		switch (ret)
		{
		default:
			Host_Error ("CL_KeepaliveMessage: CL_GetMessage failed");		
		case 0:
			break;	// nothing waiting
		case 1:
			Host_Error ("CL_KeepaliveMessage: received a message");
			break;
		case 2:
			if (net_message.ReadByte() != q1svc_nop)
				Host_Error ("CL_KeepaliveMessage: datagram wasn't a nop");
			break;
		}
	} while (ret);

	net_message.Copy(net_message._data, net_message.maxsize, old);

// check time
	time = Sys_DoubleTime ();
	if (time - lastmsg < 5)
		return;
	lastmsg = time;

// write out a nop
	Con_Printf ("--> client to server keepalive\n");

	clc.netchan.message.WriteByte(q1clc_nop);
	NET_SendMessage (cls.netcon, &clc.netchan, &clc.netchan.message);
	clc.netchan.message.Clear();
}

/*
===============
R_NewMap
===============
*/
static void R_NewMap (void)
{
	CL_ClearParticles ();

	R_EndRegistration();
}

/*
==================
CL_ParseServerInfo
==================
*/
void CL_ParseServerInfo (void)
{
	const char	*str;
	int		i;
	int		nummodels, numsounds;
	char	model_precache[MAX_MODELS][MAX_QPATH];
	char	sound_precache[MAX_SOUNDS][MAX_QPATH];
	
	Con_DPrintf ("Serverinfo packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();

// parse protocol version number
	i = net_message.ReadLong ();
	if (i != PROTOCOL_VERSION)
	{
		Con_Printf ("Server returned version %i, not %i", i, PROTOCOL_VERSION);
		return;
	}

// parse maxclients
	cl.maxclients = net_message.ReadByte ();
	if (cl.maxclients < 1 || cl.maxclients > MAX_SCOREBOARD)
	{
		Con_Printf("Bad maxclients (%u) from server\n", cl.maxclients);
		return;
	}
	cl.scores = (scoreboard_t*)Hunk_AllocName (cl.maxclients*sizeof(*cl.scores), "scores");

// parse gametype
	cl.gametype = net_message.ReadByte ();

// parse signon message
	str = net_message.ReadString2 ();
	String::NCpy(cl.levelname, str, sizeof(cl.levelname)-1);

// seperate the printfs so the server message can have a color
	Con_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Con_Printf ("%c%s\n", 2, str);

// precache models
	Com_Memset(cl.model_precache, 0, sizeof(cl.model_precache));
	for (nummodels=1 ; ; nummodels++)
	{
		str = net_message.ReadString2 ();
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
		{
			Con_Printf ("Server sent too many model precaches\n");
			return;
		}
		String::Cpy(model_precache[nummodels], str);
	}

// precache sounds
	Com_Memset(cl.sound_precache, 0, sizeof(cl.sound_precache));
	for (numsounds=1 ; ; numsounds++)
	{
		str = net_message.ReadString2 ();
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
		{
			Con_Printf ("Server sent too many sound precaches\n");
			return;
		}
		String::Cpy(sound_precache[numsounds], str);
	}

//
// now we try to load everything else until a cache allocation fails
//

	CM_LoadMap(model_precache[1], true, NULL);
	R_LoadWorld(model_precache[1]);
	CL_KeepaliveMessage();

	for (i = 2; i < nummodels; i++)
	{
		cl.model_precache[i] = R_RegisterModel(model_precache[i]);
		if (cl.model_precache[i] == 0)
		{
			Con_Printf("Model %s not found\n", model_precache[i]);
			return;
		}
		CL_KeepaliveMessage ();
	}

	S_BeginRegistration();
	for (i=1 ; i<numsounds ; i++)
	{
		cl.sound_precache[i] = S_RegisterSound(sound_precache[i]);
		CL_KeepaliveMessage ();
	}
	S_EndRegistration();


// local state
	R_NewMap ();

	Hunk_Check ();		// make sure nothing is hurt
	
	noclip_anglehack = false;		// noclip is turned off at start	
}

/*
===============
R_TranslatePlayerSkin

Translates a skin texture by the per-player color lookup
===============
*/
static void R_TranslatePlayerSkin(int playernum)
{
	int top = (cl.scores[playernum].colors & 0xf0) >> 4;
	int bottom = cl.scores[playernum].colors & 15;
	byte translate[256];
	CL_CalcQuakeSkinTranslation(top, bottom, translate);

	//
	// locate the original skin pixels
	//
	q1entity_t* ent = &clq1_entities[1 + playernum];

	R_CreateOrUpdateTranslatedModelSkinQ1(playertextures[playernum], va("*player%d", playernum), cl.model_precache[ent->state.modelindex], translate);
}

/*
==================
CL_ParseUpdate

Parse an entity update message from the server
If an entities model or origin changes from frame to frame, it must be
relinked.  Other attributes can change without relinking.
==================
*/
void CL_ParseUpdate (int bits)
{
	int			i;
	qhandle_t	model;
	int			modnum;
	qboolean	forcelink;
	q1entity_t	*ent;
	int			num;
	int			skin;

	if (clc.qh_signon == SIGNONS - 1)
	{	// first update is the final signon stage
		clc.qh_signon = SIGNONS;
		CLQ1_SignonReply ();
	}

	if (bits & Q1U_MOREBITS)
	{
		i = net_message.ReadByte ();
		bits |= (i<<8);
	}

	if (bits & Q1U_LONGENTITY)	
		num = net_message.ReadShort ();
	else
		num = net_message.ReadByte ();

	ent = CLQ1_EntityNum (num);
	const q1entity_state_t& baseline = clq1_baselines[num];

for (i=0 ; i<16 ; i++)
if (bits&(1<<i))
	bitcounts[i]++;

	if (ent->msgtime != cl.mtime[1])
		forcelink = true;	// no previous frame to lerp from
	else
		forcelink = false;

	ent->msgtime = cl.mtime[0];
	
	if (bits & Q1U_MODEL)
	{
		modnum = net_message.ReadByte ();
		if (modnum >= MAX_MODELS)
			Host_Error ("CL_ParseModel: bad modnum");
	}
	else
		modnum = baseline.modelindex;
		
	model = cl.model_precache[modnum];
	if (modnum != ent->state.modelindex)
	{
		ent->state.modelindex = modnum;
	// automatic animation (torches, etc) can be either all together
	// or randomized
		if (model)
		{
			if (R_ModelSyncType(model) == ST_RAND)
				ent->syncbase = (float)(rand()&0x7fff) / 0x7fff;
			else
				ent->syncbase = 0.0;
		}
		else
			forcelink = true;	// hack to make null model players work
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
	}
	
	if (bits & Q1U_FRAME)
		ent->state.frame = net_message.ReadByte ();
	else
		ent->state.frame = baseline.frame;

	if (bits & Q1U_COLORMAP)
		i = net_message.ReadByte();
	else
		i = baseline.colormap;
	if (i > cl.maxclients)
		Sys_Error ("i >= cl.maxclients");
	ent->state.colormap = i;

	if (bits & Q1U_SKIN)
		skin = net_message.ReadByte();
	else
		skin = baseline.skinnum;
	if (skin != ent->state.skinnum) {
		ent->state.skinnum = skin;
		if (num > 0 && num <= cl.maxclients)
			R_TranslatePlayerSkin (num - 1);
	}

	if (bits & Q1U_EFFECTS)
		ent->state.effects = net_message.ReadByte();
	else
		ent->state.effects = baseline.effects;

// shift the known values for interpolation
	VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & Q1U_ORIGIN1)
		ent->msg_origins[0][0] = net_message.ReadCoord ();
	else
		ent->msg_origins[0][0] = baseline.origin[0];
	if (bits & Q1U_ANGLE1)
		ent->msg_angles[0][0] = net_message.ReadAngle();
	else
		ent->msg_angles[0][0] = baseline.angles[0];

	if (bits & Q1U_ORIGIN2)
		ent->msg_origins[0][1] = net_message.ReadCoord ();
	else
		ent->msg_origins[0][1] = baseline.origin[1];
	if (bits & Q1U_ANGLE2)
		ent->msg_angles[0][1] = net_message.ReadAngle();
	else
		ent->msg_angles[0][1] = baseline.angles[1];

	if (bits & Q1U_ORIGIN3)
		ent->msg_origins[0][2] = net_message.ReadCoord ();
	else
		ent->msg_origins[0][2] = baseline.origin[2];
	if (bits & Q1U_ANGLE3)
		ent->msg_angles[0][2] = net_message.ReadAngle();
	else
		ent->msg_angles[0][2] = baseline.angles[2];

	if ( bits & Q1U_NOLERP )
		forcelink = true;

	if ( forcelink )
	{	// didn't have an update last message
		VectorCopy (ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy (ent->msg_origins[0], ent->state.origin);
		VectorCopy (ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy (ent->msg_angles[0], ent->state.angles);
	}
}

/*
==================
CL_ParseClientdata

Server information pertaining to this client only
==================
*/
void CL_ParseClientdata (int bits)
{
	int		i, j;
	
	if (bits & SU_VIEWHEIGHT)
		cl.viewheight = net_message.ReadChar ();
	else
		cl.viewheight = DEFAULT_VIEWHEIGHT;

	if (bits & SU_IDEALPITCH)
		cl.idealpitch = net_message.ReadChar ();
	else
		cl.idealpitch = 0;
	
	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			cl.punchangle[i] = net_message.ReadChar();
		else
			cl.punchangle[i] = 0;
		if (bits & (SU_VELOCITY1<<i) )
			cl.mvelocity[0][i] = net_message.ReadChar()*16;
		else
			cl.mvelocity[0][i] = 0;
	}

// [always sent]	if (bits & SU_ITEMS)
		i = net_message.ReadLong ();

	if (cl.items != i)
	{	// set flash times
		for (j=0 ; j<32 ; j++)
			if ( (i & (1<<j)) && !(cl.items & (1<<j)))
				cl.item_gettime[j] = cl.serverTimeFloat;
		cl.items = i;
	}
		
	cl.onground = (bits & SU_ONGROUND) != 0;
	cl.inwater = (bits & SU_INWATER) != 0;

	if (bits & SU_WEAPONFRAME)
		cl.stats[STAT_WEAPONFRAME] = net_message.ReadByte ();
	else
		cl.stats[STAT_WEAPONFRAME] = 0;

	if (bits & SU_ARMOR)
		i = net_message.ReadByte ();
	else
		i = 0;
	if (cl.stats[STAT_ARMOR] != i)
	{
		cl.stats[STAT_ARMOR] = i;
	}

	if (bits & SU_WEAPON)
		i = net_message.ReadByte ();
	else
		i = 0;
	if (cl.stats[STAT_WEAPON] != i)
	{
		cl.stats[STAT_WEAPON] = i;
	}
	
	i = net_message.ReadShort ();
	if (cl.stats[STAT_HEALTH] != i)
	{
		cl.stats[STAT_HEALTH] = i;
	}

	i = net_message.ReadByte ();
	if (cl.stats[STAT_AMMO] != i)
	{
		cl.stats[STAT_AMMO] = i;
	}

	for (i=0 ; i<4 ; i++)
	{
		j = net_message.ReadByte ();
		if (cl.stats[STAT_SHELLS+i] != j)
		{
			cl.stats[STAT_SHELLS+i] = j;
		}
	}

	i = net_message.ReadByte ();

	if (standard_quake)
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != i)
		{
			cl.stats[STAT_ACTIVEWEAPON] = i;
		}
	}
	else
	{
		if (cl.stats[STAT_ACTIVEWEAPON] != (1<<i))
		{
			cl.stats[STAT_ACTIVEWEAPON] = (1<<i);
		}
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
	if (slot > cl.maxclients)
		Sys_Error ("CL_NewTranslation: slot > cl.maxclients");
	R_TranslatePlayerSkin (slot);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	sound_num = net_message.ReadByte ();
	vol = net_message.ReadByte ();
	atten = net_message.ReadByte ();
	
	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}


#define SHOWNET(x) if(cl_shownet->value==2)Con_Printf ("%3i:%s\n", net_message.readcount-1, x);

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	int			i;
	
//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
		Con_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->value == 2)
		Con_Printf ("------------------\n");
	
	cl.onground = false;	// unless the server says otherwise	
//
// parse the message
//
	net_message.BeginReadingOOB();
	
	while (1)
	{
		if (net_message.badread)
			Host_Error ("CL_ParseServerMessage: Bad server message");

		cmd = net_message.ReadByte ();

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			return;		// end of message
		}

	// if the high bit of the command byte is set, it is a fast update
		if (cmd & Q1U_SIGNAL)
		{
			SHOWNET("fast update");
			CL_ParseUpdate (cmd&127);
			continue;
		}

		SHOWNET(svc_strings[cmd]);
	
	// other commands
		switch (cmd)
		{
		default:
			Host_Error ("CL_ParseServerMessage: Illegible server message\n");
			break;
			
		case q1svc_nop:
//			Con_Printf ("q1svc_nop\n");
			break;
			
		case q1svc_time:
			cl.mtime[1] = cl.mtime[0];
			cl.mtime[0] = net_message.ReadFloat();
			break;
			
		case q1svc_clientdata:
			i = net_message.ReadShort ();
			CL_ParseClientdata (i);
			break;
		
		case q1svc_version:
			i = net_message.ReadLong ();
			if (i != PROTOCOL_VERSION)
				Host_Error ("CL_ParseServerMessage: Server is protocol %i instead of %i\n", i, PROTOCOL_VERSION);
			break;
			
		case q1svc_disconnect:
			Host_EndGame ("Server disconnected\n");

		case q1svc_print:
			Con_Printf ("%s", net_message.ReadString2 ());
			break;
			
		case q1svc_centerprint:
			SCR_CenterPrint (net_message.ReadString2 ());
			break;
			
		case q1svc_stufftext:
			Cbuf_AddText (net_message.ReadString2 ());
			break;
			
		case q1svc_damage:
			V_ParseDamage ();
			break;
			
		case q1svc_serverinfo:
			CL_ParseServerInfo ();
			break;
			
		case q1svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[i] = net_message.ReadAngle ();
			break;
			
		case q1svc_setview:
			cl.viewentity = net_message.ReadShort ();
			break;
					
		case q1svc_lightstyle:
			i = net_message.ReadByte();
			CL_SetLightStyle(i, net_message.ReadString2());
			break;
			
		case q1svc_sound:
			CL_ParseStartSoundPacket();
			break;
			
		case q1svc_stopsound:
			i = net_message.ReadShort();
			S_StopSound(i>>3, i&7);
			break;
		
		case q1svc_updatename:
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: q1svc_updatename > MAX_SCOREBOARD");
			String::Cpy(cl.scores[i].name, net_message.ReadString2 ());
			break;
			
		case q1svc_updatefrags:
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: q1svc_updatefrags > MAX_SCOREBOARD");
			cl.scores[i].frags = net_message.ReadShort ();
			break;			

		case q1svc_updatecolors:
			i = net_message.ReadByte ();
			if (i >= cl.maxclients)
				Host_Error ("CL_ParseServerMessage: q1svc_updatecolors > MAX_SCOREBOARD");
			cl.scores[i].colors = net_message.ReadByte ();
			CL_NewTranslation (i);
			break;
			
		case q1svc_particle:
			R_ParseParticleEffect ();
			break;

		case q1svc_spawnbaseline:
			CLQ1_ParseSpawnBaseline(net_message);
			break;
		case q1svc_spawnstatic:
			CLQ1_ParseSpawnStatic(net_message);
			break;			
		case q1svc_temp_entity:
			CLQ1_ParseTEnt (net_message);
			break;

		case q1svc_setpause:
			{
				cl.paused = net_message.ReadByte ();

				if (cl.paused)
				{
					CDAudio_Pause ();
				}
				else
				{
					CDAudio_Resume ();
				}
			}
			break;
			
		case q1svc_signonnum:
			i = net_message.ReadByte ();
			if (i <= clc.qh_signon)
				Host_Error ("Received signon %i when at %i", i, clc.qh_signon);
			clc.qh_signon = i;
			CLQ1_SignonReply ();
			break;

		case q1svc_killedmonster:
			cl.stats[STAT_MONSTERS]++;
			break;

		case q1svc_foundsecret:
			cl.stats[STAT_SECRETS]++;
			break;

		case q1svc_updatestat:
			i = net_message.ReadByte ();
			if (i < 0 || i >= MAX_CL_STATS)
				Sys_Error ("q1svc_updatestat: %i is invalid", i);
			cl.stats[i] = net_message.ReadLong ();;
			break;
			
		case q1svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case q1svc_cdtrack:
			cl.cdtrack = net_message.ReadByte ();
			cl.looptrack = net_message.ReadByte ();
			if ( (cls.demoplayback || cls.demorecording) && (cls.forcetrack != -1) )
				CDAudio_Play ((byte)cls.forcetrack, true);
			else
				CDAudio_Play ((byte)cl.cdtrack, true);
			break;

		case q1svc_intermission:
			cl.intermission = 1;
			cl.completed_time = cl.serverTimeFloat;
			break;

		case q1svc_finale:
			cl.intermission = 2;
			cl.completed_time = cl.serverTimeFloat;
			SCR_CenterPrint (net_message.ReadString2 ());			
			break;

		case q1svc_cutscene:
			cl.intermission = 3;
			cl.completed_time = cl.serverTimeFloat;
			SCR_CenterPrint (net_message.ReadString2 ());			
			break;

		case q1svc_sellscreen:
			Cmd_ExecuteString ("help", src_command);
			break;
		}
	}
}

