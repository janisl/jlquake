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
// sv_user.c -- server code for moving users

#include "server.h"

edict_t	*sv_player;

/*
============================================================

USER STRINGCMD EXECUTION

sv_client and sv_player will be valid.
============================================================
*/

/*
==================
SV_BeginDemoServer
==================
*/
void SV_BeginDemoserver (void)
{
	char		name[MAX_OSPATH];

	String::Sprintf (name, sizeof(name), "demos/%s", sv.name);
	FS_FOpenFileRead(name, &sv.demofile, true);
	if (!sv.demofile)
		Com_Error (ERR_DROP, "Couldn't open %s\n", name);
}

/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f (void)
{
	const char	*gamedir;
	int			playernum;
	edict_t		*ent;

	Com_DPrintf ("New() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("New not valid -- already spawned\n");
		return;
	}

	// demo servers just dump the file message
	if (sv.state == ss_demo)
	{
		SV_BeginDemoserver ();
		return;
	}

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	gamedir = Cvar_VariableString ("gamedir");

	// send the serverdata
	sv_client->netchan.message.WriteByte(q2svc_serverdata);
	sv_client->netchan.message.WriteLong(PROTOCOL_VERSION);
	sv_client->netchan.message.WriteLong(svs.spawncount);
	sv_client->netchan.message.WriteByte(sv.attractloop);
	sv_client->netchan.message.WriteString2(gamedir);

	if (sv.state == ss_cinematic || sv.state == ss_pic)
		playernum = -1;
	else
		playernum = sv_client - svs.clients;
	sv_client->netchan.message.WriteShort(playernum);

	// send full levelname
	sv_client->netchan.message.WriteString2(sv.configstrings[CS_NAME]);

	//
	// game server
	// 
	if (sv.state == ss_game)
	{
		// set up the entity for the client
		ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		sv_client->edict = ent;
		Com_Memset(&sv_client->lastcmd, 0, sizeof(sv_client->lastcmd));

		// begin fetching configstrings
		sv_client->netchan.message.WriteByte(q2svc_stufftext);
		sv_client->netchan.message.WriteString2(va("cmd configstrings %i 0\n",svs.spawncount) );
	}

}

/*
==================
SV_Configstrings_f
==================
*/
void SV_Configstrings_f (void)
{
	int			start;

	Com_DPrintf ("Configstrings() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("configstrings not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( String::Atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Configstrings_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = String::Atoi(Cmd_Argv(2));

	// write a packet full of data

	while ( sv_client->netchan.message.cursize < MAX_MSGLEN_Q2/2 
		&& start < MAX_CONFIGSTRINGS)
	{
		if (sv.configstrings[start][0])
		{
			sv_client->netchan.message.WriteByte(q2svc_configstring);
			sv_client->netchan.message.WriteShort(start);
			sv_client->netchan.message.WriteString2(sv.configstrings[start]);
		}
		start++;
	}

	// send next command

	if (start == MAX_CONFIGSTRINGS)
	{
		sv_client->netchan.message.WriteByte(q2svc_stufftext);
		sv_client->netchan.message.WriteString2(va("cmd baselines %i 0\n",svs.spawncount) );
	}
	else
	{
		sv_client->netchan.message.WriteByte(q2svc_stufftext);
		sv_client->netchan.message.WriteString2(va("cmd configstrings %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Baselines_f
==================
*/
void SV_Baselines_f (void)
{
	int		start;
	q2entity_state_t	nullstate;
	q2entity_state_t	*base;

	Com_DPrintf ("Baselines() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("baselines not valid -- already spawned\n");
		return;
	}
	
	// handle the case of a level changing while a client was connecting
	if ( String::Atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Baselines_f from different level\n");
		SV_New_f ();
		return;
	}
	
	start = String::Atoi(Cmd_Argv(2));

	Com_Memset(&nullstate, 0, sizeof(nullstate));

	// write a packet full of data

	while ( sv_client->netchan.message.cursize <  MAX_MSGLEN_Q2/2
		&& start < MAX_EDICTS_Q2)
	{
		base = &sv.baselines[start];
		if (base->modelindex || base->sound || base->effects)
		{
			sv_client->netchan.message.WriteByte(q2svc_spawnbaseline);
			MSG_WriteDeltaEntity (&nullstate, base, &sv_client->netchan.message, true, true);
		}
		start++;
	}

	// send next command

	if (start == MAX_EDICTS_Q2)
	{
		sv_client->netchan.message.WriteByte(q2svc_stufftext);
		sv_client->netchan.message.WriteString2(va("precache %i\n", svs.spawncount) );
	}
	else
	{
		sv_client->netchan.message.WriteByte(q2svc_stufftext);
		sv_client->netchan.message.WriteString2(va("cmd baselines %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f (void)
{
	Com_DPrintf ("Begin() from %s\n", sv_client->name);

	// handle the case of a level changing while a client was connecting
	if ( String::Atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Begin_f from different level\n");
		SV_New_f ();
		return;
	}

	sv_client->state = cs_spawned;

	// call the game begin function
	ge->ClientBegin (sv_player);

	Cbuf_InsertFromDefer ();
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f (void)
{
	int		r;
	int		percent;
	int		size;

	if (!sv_client->download)
		return;

	r = sv_client->downloadsize - sv_client->downloadcount;
	if (r > 1024)
		r = 1024;

	sv_client->netchan.message.WriteByte(q2svc_download);
	sv_client->netchan.message.WriteShort(r);

	sv_client->downloadcount += r;
	size = sv_client->downloadsize;
	if (!size)
		size = 1;
	percent = sv_client->downloadcount*100/size;
	sv_client->netchan.message.WriteByte(percent);
	sv_client->netchan.message.WriteData(
		sv_client->download + sv_client->downloadcount - r, r);

	if (sv_client->downloadcount != sv_client->downloadsize)
		return;

	FS_FreeFile (sv_client->download);
	sv_client->download = NULL;
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(void)
{
	char	*name;
	extern	Cvar *allow_download;
	extern	Cvar *allow_download_players;
	extern	Cvar *allow_download_models;
	extern	Cvar *allow_download_sounds;
	extern	Cvar *allow_download_maps;
	int offset = 0;

	name = Cmd_Argv(1);

	if (Cmd_Argc() > 2)
		offset = String::Atoi(Cmd_Argv(2)); // downloaded offset

	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if (strstr (name, "..") || !allow_download->value
		// leading dot is no good
		|| *name == '.' 
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// next up, skin check
		|| (String::NCmp(name, "players/", 6) == 0 && !allow_download_players->value)
		// now models
		|| (String::NCmp(name, "models/", 6) == 0 && !allow_download_models->value)
		// now sounds
		|| (String::NCmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)
		// now maps (note special case for maps, must not be in pak)
		|| (String::NCmp(name, "maps/", 6) == 0 && !allow_download_maps->value)
		// MUST be in a subdirectory	
		|| !strstr (name, "/") )	
	{	// don't allow anything with .. path
		sv_client->netchan.message.WriteByte(q2svc_download);
		sv_client->netchan.message.WriteShort(-1);
		sv_client->netchan.message.WriteByte(0);
		return;
	}


	if (sv_client->download)
		FS_FreeFile (sv_client->download);

	sv_client->downloadsize = FS_ReadFile(name, (void **)&sv_client->download);
	sv_client->downloadcount = offset;

	if (offset > sv_client->downloadsize)
		sv_client->downloadcount = sv_client->downloadsize;

	if (!sv_client->download
		// special check for maps, if it came from a pak file, don't allow
		// download  ZOID
		|| (String::NCmp(name, "maps/", 5) == 0 && FS_FileIsInPAK(name, NULL) == 1))
	{
		Com_DPrintf ("Couldn't download %s to %s\n", name, sv_client->name);
		if (sv_client->download) {
			FS_FreeFile (sv_client->download);
			sv_client->download = NULL;
		}

		sv_client->netchan.message.WriteByte(q2svc_download);
		sv_client->netchan.message.WriteShort(-1);
		sv_client->netchan.message.WriteByte(0);
		return;
	}

	SV_NextDownload_f ();
	Com_DPrintf ("Downloading %s to %s\n", name, sv_client->name);
}



//============================================================================


/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Disconnect_f (void)
{
//	SV_EndRedirect ();
	SV_DropClient (sv_client);	
}


/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f (void)
{
	Info_Print (Cvar_InfoString(CVAR_SERVERINFO, MAX_INFO_STRING, MAX_INFO_KEY,
		MAX_INFO_VALUE, true, false));
}


void SV_Nextserver (void)
{
	const char	*v;

	//ZOID, ss_pic can be nextserver'd in coop mode
	if (sv.state == ss_game || (sv.state == ss_pic && !Cvar_VariableValue("coop")))
		return;		// can't nextserver while playing a normal game

	svs.spawncount++;	// make sure another doesn't sneak in
	v = Cvar_VariableString ("nextserver");
	if (!v[0])
		Cbuf_AddText ("killserver\n");
	else
	{
		Cbuf_AddText (const_cast<char*>(v));
		Cbuf_AddText ("\n");
	}
	Cvar_SetLatched("nextserver","");
}

/*
==================
SV_Nextserver_f

A cinematic has completed or been aborted by a client, so move
to the next server,
==================
*/
void SV_Nextserver_f (void)
{
	if ( String::Atoi(Cmd_Argv(1)) != svs.spawncount ) {
		Com_DPrintf ("Nextserver() from wrong level, from %s\n", sv_client->name);
		return;		// leftover from last server
	}

	Com_DPrintf ("Nextserver() from %s\n", sv_client->name);

	SV_Nextserver ();
}

typedef struct
{
	const char	*name;
	void	(*func) (void);
} ucmd_t;

ucmd_t ucmds[] =
{
	// auto issued
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	// issued by hand at client consoles	
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteUserCommand (char *s)
{
	ucmd_t	*u;
	
	Cmd_TokenizeString (s, true);
	sv_player = sv_client->edict;

//	SV_BeginRedirect (RD_CLIENT);

	for (u=ucmds ; u->name ; u++)
		if (!String::Cmp(Cmd_Argv(0), u->name) )
		{
			u->func ();
			break;
		}

	if (!u->name && sv.state == ss_game)
		ge->ClientCommand (sv_player);

//	SV_EndRedirect ();
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/



void SV_ClientThink (client_t *cl, q2usercmd_t *cmd)

{
	cl->commandMsec -= cmd->msec;

	if (cl->commandMsec < 0 && sv_enforcetime->value )
	{
		Com_DPrintf ("commandMsec underflow from %s\n", cl->name);
		return;
	}

	ge->ClientThink (cl->edict, cmd);
}



#define	MAX_STRINGCMDS	8
/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
void SV_ExecuteClientMessage (client_t *cl)
{
	int		c;
	char	*s;

	q2usercmd_t	nullcmd;
	q2usercmd_t	oldest, oldcmd, newcmd;
	int		net_drop;
	int		stringCmdCount;
	int		checksum, calculatedChecksum;
	int		checksumIndex;
	qboolean	move_issued;
	int		lastframe;

	sv_client = cl;
	sv_player = sv_client->edict;

	// only allow one move command
	move_issued = false;
	stringCmdCount = 0;

	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Printf ("SV_ReadClientMessage: badread\n");
			SV_DropClient (cl);
			return;
		}	

		c = net_message.ReadByte();
		if (c == -1)
			break;
				
		switch (c)
		{
		default:
			Com_Printf ("SV_ReadClientMessage: unknown command char\n");
			SV_DropClient (cl);
			return;
						
		case q2clc_nop:
			break;

		case q2clc_userinfo:
			String::NCpy(cl->userinfo, net_message.ReadString2(), sizeof(cl->userinfo)-1);
			SV_UserinfoChanged (cl);
			break;

		case q2clc_move:
			if (move_issued)
				return;		// someone is trying to cheat...

			move_issued = true;
			checksumIndex = net_message.readcount;
			checksum = net_message.ReadByte();
			lastframe = net_message.ReadLong();
			if (lastframe != cl->lastframe) {
				cl->lastframe = lastframe;
				if (cl->lastframe > 0) {
					cl->frame_latency[cl->lastframe&(LATENCY_COUNTS-1)] = 
						svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK_Q2].senttime;
				}
			}

			Com_Memset(&nullcmd, 0, sizeof(nullcmd));
			MSG_ReadDeltaUsercmd (&net_message, &nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&net_message, &oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&net_message, &oldcmd, &newcmd);

			if ( cl->state != cs_spawned )
			{
				cl->lastframe = -1;
				break;
			}

			// if the checksum fails, ignore the rest of the packet
			calculatedChecksum = COM_BlockSequenceCRCByte (
				net_message._data + checksumIndex + 1,
				net_message.readcount - checksumIndex - 1,
				cl->netchan.incomingSequence);

			if (calculatedChecksum != checksum)
			{
				Com_DPrintf ("Failed command checksum for %s (%d != %d)/%d\n", 
					cl->name, calculatedChecksum, checksum, 
					cl->netchan.incomingSequence);
				return;
			}

			if (!sv_paused->value)
			{
				net_drop = cl->netchan.dropped;
				if (net_drop < 20)
				{

//if (net_drop > 2)

//	Com_Printf ("drop %i\n", net_drop);
					while (net_drop > 2)
					{
						SV_ClientThink (cl, &cl->lastcmd);

						net_drop--;
					}
					if (net_drop > 1)
						SV_ClientThink (cl, &oldest);

					if (net_drop > 0)
						SV_ClientThink (cl, &oldcmd);

				}
				SV_ClientThink (cl, &newcmd);
			}

			cl->lastcmd = newcmd;
			break;

		case q2clc_stringcmd:	
			s = const_cast<char*>(net_message.ReadString2());

			// malicious users may try using too many string commands
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand (s);

			if (cl->state == cs_zombie)
				return;	// disconnect command
			break;
		}
	}
}

