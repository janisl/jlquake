
#include "qwsvdef.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif
quakeparms_t host_parms;

qboolean host_initialized;			// true if into command execution (compatability)

double host_frametime;
double realtime;					// without any filtering or bounding

int host_hunklevel;

netadr_t idmaster_adr;					// for global logging

client_t* host_client;				// current client

Cvar* timeout;
Cvar* zombietime;

Cvar* rcon_password;
Cvar* password;
Cvar* spectator_password;

Cvar* allow_download;
Cvar* allow_download_skins;
Cvar* allow_download_models;
Cvar* allow_download_sounds;
Cvar* allow_download_maps;

Cvar* sv_namedistance;


//
// game rules mirrored in svs.qh_info
//
Cvar* fraglimit;
Cvar* timelimit;
Cvar* samelevel;
Cvar* maxspectators;
Cvar* skill;
Cvar* randomclass;
Cvar* damageScale;
Cvar* shyRespawn;
Cvar* spartanPrint;
Cvar* meleeDamScale;
Cvar* manaScale;
Cvar* tomeMode;
Cvar* tomeRespawn;
Cvar* w2Respawn;
Cvar* altRespawn;
Cvar* fixedLevel;
Cvar* autoItems;
Cvar* dmMode;
Cvar* easyFourth;
Cvar* patternRunner;
Cvar* spawn;

Cvar* sv_ce_scale;
Cvar* sv_ce_max_size;

Cvar* noexit;

fileHandle_t sv_logfile;

int sv_net_port;

void SV_AcceptClient(netadr_t adr, int userid, char* userinfo);
void Master_Shutdown(void);

class QMainLog : public LogListener
{
public:
	void serialise(const char* text)
	{
		Con_Printf("%s", text);
	}
	void develSerialise(const char* text)
	{
		Con_DPrintf("%s", text);
	}
} MainLog;

void S_ClearSoundBuffer(bool killStreaming)
{
}

//============================================================================

/*
================
SV_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SV_Shutdown(void)
{
	Master_Shutdown();
	if (sv_logfile)
	{
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
	}
	if (svqhw_fraglogfile)
	{
		FS_FCloseFile(svqhw_fraglogfile);
		svqhw_fraglogfile = 0;
	}
	NET_Shutdown();
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void SV_Error(const char* error, ...)
{
	va_list argptr;
	static char string[1024];
	static qboolean inerror = false;

	if (inerror)
	{
		Sys_Error("SV_Error: recursively entered (%s)", string);
	}

	inerror = true;

	va_start(argptr,error);
	Q_vsnprintf(string, 1024, error, argptr);
	va_end(argptr);

	Con_Printf("SV_Error: %s\n",string);

	SV_FinalMessage(va("server crashed: %s\n", string));

	SV_Shutdown();

	Sys_Error("SV_Error: %s\n",string);
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
void SV_FinalMessage(const char* message)
{
	int i;
	client_t* cl;

	net_message.Clear();
	net_message.WriteByte(h2svc_print);
	net_message.WriteByte(PRINT_HIGH);
	net_message.WriteString2(message);
	net_message.WriteByte(h2svc_disconnect);

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
		if (cl->state >= CS_ACTIVE)
		{
			Netchan_Transmit(&cl->netchan, net_message.cursize,
				net_message._data);
		}
}

/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient(client_t* drop)
{

	// add the disconnect
	drop->netchan.message.WriteByte(h2svc_disconnect);

	if (drop->state == CS_ACTIVE)
	{
		if (!drop->qh_spectator)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			*pr_globalVars.self = EDICT_TO_PROG(drop->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.ClientDisconnect);
		}
		else if (SpectatorDisconnect)
		{
			// call the prog function for removing a client
			// this will set the body to a dead frame, among other things
			*pr_globalVars.self = EDICT_TO_PROG(drop->qh_edict);
			PR_ExecuteProgram(SpectatorDisconnect);
		}
	}
	else if (dmMode->value == DM_SIEGE)
	{
		if (String::ICmp(PR_GetString(drop->qh_edict->GetPuzzleInv1()),""))
		{
			//this guy has a puzzle piece, call this function anyway
			//to make sure he leaves it behind
			Con_Printf("Client in unspawned state had puzzle piece, forcing drop\n");
			*pr_globalVars.self = EDICT_TO_PROG(drop->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.ClientDisconnect);
		}
	}

	if (drop->qh_spectator)
	{
		Con_Printf("Spectator %s removed\n",drop->name);
	}
	else
	{
		Con_Printf("Client %s removed\n",drop->name);
	}

	if (drop->download)
	{
		FS_FCloseFile(drop->download);
		drop->download = 0;
	}

	drop->state = CS_ZOMBIE;		// become free in a few seconds
	drop->qh_connection_started = realtime;	// for zombie timeout

	drop->qh_old_frags = 0;
	drop->qh_edict->SetFrags(0);
	drop->name[0] = 0;
	Com_Memset(drop->userinfo, 0, sizeof(drop->userinfo));

// send notification to all remaining clients
	SV_FullClientUpdate(drop, &sv.qh_reliable_datagram);
}


//====================================================================

/*
===================
SV_CalcPing

===================
*/
int SV_CalcPing(client_t* cl)
{
	float ping;
	int i;
	int count;
	register hwclient_frame_t* frame;

	ping = 0;
	count = 0;
	for (frame = cl->hw_frames, i = 0; i < UPDATE_BACKUP_HW; i++, frame++)
	{
		if (frame->ping_time > 0)
		{
			ping += frame->ping_time;
			count++;
		}
	}
	if (!count)
	{
		return 9999;
	}
	ping /= count;

	return ping * 1000;
}

/*
===================
SV_FullClientUpdate

Writes all update values to a sizebuf
===================
*/
unsigned int defLosses;	// Defenders losses in Siege
unsigned int attLosses;	// Attackers Losses in Siege
void SV_FullClientUpdate(client_t* client, QMsg* buf)
{
	int i;
	char info[HWMAX_INFO_STRING];

//	Con_Printf("SV_FullClientUpdate\n");
	i = client - svs.clients;

//Con_Printf("SV_FullClientUpdate:  Updated frags for client %d\n", i);

	buf->WriteByte(hwsvc_updatedminfo);
	buf->WriteByte(i);
	buf->WriteShort(client->qh_old_frags);
	buf->WriteByte((client->h2_playerclass << 5) | ((int)client->qh_edict->GetLevel() & 31));

	if (dmMode->value == DM_SIEGE)
	{
		buf->WriteByte(hwsvc_updatesiegeinfo);
		buf->WriteByte((int)ceil(timelimit->value));
		buf->WriteByte((int)ceil(fraglimit->value));

		buf->WriteByte(hwsvc_updatesiegeteam);
		buf->WriteByte(i);
		buf->WriteByte(client->hw_siege_team);

		buf->WriteByte(hwsvc_updatesiegelosses);
		buf->WriteByte(*pr_globalVars.defLosses);
		buf->WriteByte(*pr_globalVars.attLosses);

		buf->WriteByte(h2svc_time);	//send server time upon connection
		buf->WriteFloat(sv.qh_time);
	}

	buf->WriteByte(hwsvc_updateping);
	buf->WriteByte(i);
	buf->WriteShort(SV_CalcPing(client));

	buf->WriteByte(hwsvc_updateentertime);
	buf->WriteByte(i);
	buf->WriteFloat(realtime - client->qh_connection_started);

	String::Cpy(info, client->userinfo);
	Info_RemovePrefixedKeys(info, '_', HWMAX_INFO_STRING);	// server passwords, etc

	buf->WriteByte(hwsvc_updateuserinfo);
	buf->WriteByte(i);
	buf->WriteLong(client->qh_userid);
	buf->WriteString2(info);
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
void SVC_Status(void)
{
	int i;
	client_t* cl;
	int ping;
	int top, bottom;

	Cmd_TokenizeString("status");
	SV_BeginRedirect(RD_PACKET);
	Con_Printf("%s\n", svs.qh_info);
	for (i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state == CS_CONNECTED || cl->state == CS_ACTIVE) && !cl->qh_spectator)
		{
			top = String::Atoi(Info_ValueForKey(cl->userinfo, "topcolor"));
			bottom = String::Atoi(Info_ValueForKey(cl->userinfo, "bottomcolor"));
			ping = SV_CalcPing(cl);
			Con_Printf("%i %i %i %i \"%s\" \"%s\" %i %i\n", cl->qh_userid,
				cl->qh_old_frags, (int)(realtime - cl->qh_connection_started) / 60,
				ping, cl->name, Info_ValueForKey(cl->userinfo, "skin"), top, bottom);
		}
	}
	SV_EndRedirect();
}

/*
===================
SV_CheckLog

===================
*/
#define LOG_HIGHWATER   4096
#define LOG_FLUSH       10 * 60
void SV_CheckLog(void)
{
	QMsg* sz;

	sz = &svs.qh_log[svs.qh_logsequence & 1];

	// bump sequence if allmost full, or ten minutes have passed and
	// there is something still sitting there
	if (sz->cursize > LOG_HIGHWATER ||
		(realtime - svs.qh_logtime > LOG_FLUSH && sz->cursize))
	{
		// swap buffers and bump sequence
		svs.qh_logtime = realtime;
		svs.qh_logsequence++;
		sz = &svs.qh_log[svs.qh_logsequence & 1];
		sz->cursize = 0;
		Con_Printf("beginning fraglog sequence %i\n", svs.qh_logsequence);
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
void SVC_Log(void)
{
	int seq;
	char data[MAX_DATAGRAM_HW + 64];

	if (Cmd_Argc() == 2)
	{
		seq = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		seq = -1;
	}

	if (seq == svs.qh_logsequence - 1 || !svqhw_fraglogfile)
	{	// they allready have this data, or we aren't logging frags
		data[0] = A2A_NACK;
		NET_SendPacket(1, data, net_from);
		return;
	}

	Con_DPrintf("sending log %i to %s\n", svs.qh_logsequence - 1, SOCK_AdrToString(net_from));

	sprintf(data, "stdlog %i\n", svs.qh_logsequence - 1);
	String::Cat(data, sizeof(data), (char*)svs.qh_log_buf[((svs.qh_logsequence - 1) & 1)]);

	NET_SendPacket(String::Length(data) + 1, data, net_from);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping(void)
{
	char data;

	data = A2A_ACK;

	NET_SendPacket(1, &data, net_from);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect(void)
{
	char userinfo[1024];
	static int userid;
	netadr_t adr;
	int i;
	client_t* cl, * newcl;
	client_t temp;
	qhedict_t* ent;
	int edictnum;
	const char* s;
	int clients, spectators;
	qboolean spectator;

	String::NCpy(userinfo, Cmd_Argv(2), sizeof(userinfo) - 1);

	// check for password or spectator_password
	s = Info_ValueForKey(userinfo, "spectator");
	if (s[0] && String::Cmp(s, "0"))
	{
		if (spectator_password->string[0] &&
			String::ICmp(spectator_password->string, "none") &&
			String::Cmp(spectator_password->string, s))
		{	// failed
			Con_Printf("%s:spectator password failed\n", SOCK_AdrToString(net_from));
			Netchan_OutOfBandPrint(net_from, "%c\nrequires a spectator password\n\n", A2C_PRINT);
			return;
		}
		Info_SetValueForKey(userinfo, "*spectator", "1", HWMAX_INFO_STRING, 64, 64, !svqh_highchars->value);
		spectator = true;
		Info_RemoveKey(userinfo, "spectator", HWMAX_INFO_STRING);	// remove passwd
	}
	else
	{
		s = Info_ValueForKey(userinfo, "password");
		if (password->string[0] &&
			String::ICmp(password->string, "none") &&
			String::Cmp(password->string, s))
		{
			Con_Printf("%s:password failed\n", SOCK_AdrToString(net_from));
			Netchan_OutOfBandPrint(net_from, "%c\nserver requires a password\n\n", A2C_PRINT);
			return;
		}
		spectator = false;
		Info_RemoveKey(userinfo, "password", HWMAX_INFO_STRING);	// remove passwd
	}

	adr = net_from;
	userid++;	// so every client gets a unique id

	newcl = &temp;
	Com_Memset(newcl, 0, sizeof(client_t));

	newcl->qh_userid = userid;
	newcl->hw_portals = atol(Cmd_Argv(1));

	// works properly
	if (!svqh_highchars->value)
	{
		char* p, * q;

		for (p = newcl->userinfo, q = userinfo; *q; q++)
			if (*q > 31 && *q <= 127)
			{
				*p++ = *q;
			}
	}
	else
	{
		String::NCpy(newcl->userinfo, userinfo, HWMAX_INFO_STRING - 1);
	}

	// if there is allready a slot for this ip, drop it
	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (SOCK_CompareAdr(adr, cl->netchan.remoteAddress))
		{
			Con_Printf("%s:reconnect\n", SOCK_AdrToString(adr));
			SV_DropClient(cl);
			break;
		}
	}

	// count up the clients and spectators
	clients = 0;
	spectators = 0;
	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (cl->qh_spectator)
		{
			spectators++;
		}
		else
		{
			clients++;
		}
	}

	// if at server limits, refuse connection
	if (sv_maxclients->value > MAX_CLIENTS_QHW)
	{
		Cvar_SetValue("maxclients", MAX_CLIENTS_QHW);
	}
	if (maxspectators->value > MAX_CLIENTS_QHW)
	{
		Cvar_SetValue("maxspectators", MAX_CLIENTS_QHW);
	}
	if (maxspectators->value + sv_maxclients->value > MAX_CLIENTS_QHW)
	{
		Cvar_SetValue("maxspectators", MAX_CLIENTS_QHW - maxspectators->value + sv_maxclients->value);
	}
	if ((spectator && spectators >= (int)maxspectators->value) ||
		(!spectator && clients >= (int)sv_maxclients->value))
	{
		Con_Printf("%s:full connect\n", SOCK_AdrToString(adr));
		Netchan_OutOfBandPrint(adr, "%c\nserver is full\n\n", A2C_PRINT);
		return;
	}

	// find a client slot
	newcl = NULL;
	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		Con_Printf("WARNING: miscounted available clients\n");
		return;
	}


	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;

	Netchan_OutOfBandPrint(adr, "%c", S2C_CONNECTION);

	edictnum = (newcl - svs.clients) + 1;

	Netchan_Setup(NS_SERVER, &newcl->netchan, adr);

	newcl->state = CS_CONNECTED;

	newcl->datagram.InitOOB(newcl->datagramBuffer, MAX_DATAGRAM_HW);
	newcl->datagram.allowoverflow = true;

	// spectator mode can ONLY be set at join time
	newcl->qh_spectator = spectator;

	ent = QH_EDICT_NUM(edictnum);
	newcl->qh_edict = ent;
	ED_ClearEdict(ent);

	// parse some info from the info strings
	SV_ExtractFromUserinfo(newcl);

	// JACK: Init the floodprot stuff.
	for (i = 0; i < 10; i++)
		newcl->qh_whensaid[i] = 0.0;
	newcl->qh_whensaidhead = 0;
	newcl->qh_lockedtill = 0;

	// call the progs to get default spawn parms for the new client
	PR_ExecuteProgram(*pr_globalVars.SetNewParms);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
		newcl->qh_spawn_parms[i] = pr_globalVars.parm1[i];

	if (newcl->qh_spectator)
	{
		Con_Printf("Spectator %s connected\n", newcl->name);
	}
	else
	{
		Con_DPrintf("Client %s connected\n", newcl->name);
	}
}

int Rcon_Validate(void)
{
	if (net_from.ip[0] == 208 && net_from.ip[1] == 135 && net_from.ip[2] == 137
//		&& !String::Cmp(Cmd_Argv(1), "tms") )
		&& !String::Cmp(Cmd_Argv(1), "rjr"))
	{
		return 2;
	}

	if (!String::Length(rcon_password->string))
	{
		return 0;
	}

	if (String::Cmp(Cmd_Argv(1), rcon_password->string))
	{
		return 0;
	}

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
void SVC_RemoteCommand(void)
{
	int i;
	char remaining[1024];

	i = Rcon_Validate();

	if (i == 0)
	{
		Con_Printf("Bad rcon from %s:\n%s\n",
			SOCK_AdrToString(net_from), net_message._data + 4);
	}
	if (i == 1)
	{
		Con_Printf("Rcon from %s:\n%s\n",
			SOCK_AdrToString(net_from), net_message._data + 4);
	}

	SV_BeginRedirect(RD_PACKET);

	if (!Rcon_Validate())
	{
		Con_Printf("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++)
		{
			String::Cat(remaining, sizeof(remaining), Cmd_Argv(i));
			String::Cat(remaining, sizeof(remaining), " ");
		}

		Cmd_ExecuteString(remaining);
	}

	SV_EndRedirect();
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
void SV_ConnectionlessPacket(void)
{
	char* s;
	char* c;

	net_message.BeginReadingOOB();
	net_message.ReadLong();		// skip the -1 marker

	s = const_cast<char*>(net_message.ReadStringLine2());

	Cmd_TokenizeString(s);

	c = Cmd_Argv(0);

	if (!String::Cmp(c, "ping") || (c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')))
	{
		SVC_Ping();
		return;
	}
	if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n'))
	{
		Con_Printf("A2A_ACK from %s\n", SOCK_AdrToString(net_from));
		return;
	}
	else if (c[0] == A2S_ECHO)
	{
		NET_SendPacket(net_message.cursize, net_message._data, net_from);
		return;
	}
	else if (!String::Cmp(c,"status"))
	{
		SVC_Status();
		return;
	}
	else if (!String::Cmp(c,"log"))
	{
		SVC_Log();
		return;
	}
	else if (!String::Cmp(c,"connect"))
	{
		SVC_DirectConnect();
		return;
	}
	else if (!String::Cmp(c, "rcon"))
	{
		SVC_RemoteCommand();
	}
	else
	{
		Con_Printf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(net_from), s);
	}
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
	unsigned mask;
	unsigned compare;
} ipfilter_t;

#define MAX_IPFILTERS   1024

ipfilter_t ipfilters[MAX_IPFILTERS];
int numipfilters;

Cvar* filterban;

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter(char* s, ipfilter_t* f)
{
	char num[128];
	int i, j;
	byte b[4];
	byte m[4];

	for (i = 0; i < 4; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0; i < 4; i++)
	{
		if (*s < '0' || *s > '9')
		{
			Con_Printf("Bad filter address: %s\n", s);
			return false;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = String::Atoi(num);
		if (b[i] != 0)
		{
			m[i] = 255;
		}

		if (!*s)
		{
			break;
		}
		s++;
	}

	f->mask = *(unsigned*)m;
	f->compare = *(unsigned*)b;

	return true;
}

/*
=================
SV_AddIP_f
=================
*/
void SV_AddIP_f(void)
{
	int i;

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].compare == 0xffffffff)
		{
			break;
		}				// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			Con_Printf("IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter(Cmd_Argv(1), &ipfilters[i]))
	{
		ipfilters[i].compare = 0xffffffff;
	}
}

/*
=================
SV_RemoveIP_f
=================
*/
void SV_RemoveIP_f(void)
{
	ipfilter_t f;
	int i, j;

	if (!StringToFilter(Cmd_Argv(1), &f))
	{
		return;
	}
	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].mask == f.mask &&
			ipfilters[i].compare == f.compare)
		{
			for (j = i + 1; j < numipfilters; j++)
				ipfilters[j - 1] = ipfilters[j];
			numipfilters--;
			Con_Printf("Removed.\n");
			return;
		}
	Con_Printf("Didn't find %s.\n", Cmd_Argv(1));
}

/*
=================
SV_ListIP_f
=================
*/
void SV_ListIP_f(void)
{
	int i;
	byte b[4];

	Con_Printf("Filter list:\n");
	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;
		Con_Printf("%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
void SV_WriteIP_f(void)
{
	fileHandle_t f;
	char name[MAX_OSPATH];
	byte b[4];
	int i;

	sprintf(name, "listip.cfg");

	Con_Printf("Writing %s.\n", name);

	f = FS_FOpenFileWrite(name);
	if (!f)
	{
		Con_Printf("Couldn't open %s\n", name);
		return;
	}

	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;
		FS_Printf(f, "addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}

	FS_FCloseFile(f);
}

/*
=================
SV_SendBan
=================
*/
void SV_SendBan(void)
{
	char data[128];

	data[0] = data[1] = data[2] = data[3] = 0xff;
	data[4] = A2C_PRINT;
	data[5] = 0;
	String::Cat(data, sizeof(data), "\nbanned.\n");

	NET_SendPacket(String::Length(data), data, net_from);
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket(void)
{
	int i;
	unsigned in;

	in = *(unsigned*)net_from.ip;

	for (i = 0; i < numipfilters; i++)
		if ((in & ipfilters[i].mask) == ipfilters[i].compare)
		{
			return filterban->value;
		}

	return !filterban->value;
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets(void)
{
	int i;
	client_t* cl;
	qboolean good;

	good = false;
	while (NET_GetUdpPacket(NS_SERVER, &net_from, &net_message))
	{
		if (SV_FilterPacket())
		{
			SV_SendBan();	// tell them we aren't listening...
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int*)net_message._data == -1)
		{
			SV_ConnectionlessPacket();
			continue;
		}

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (cl->state == CS_FREE)
			{
				continue;
			}
			if (!SOCK_CompareAdr(net_from, cl->netchan.remoteAddress))
			{
				continue;
			}
			if (Netchan_Process(&cl->netchan))
			{	// this is a valid, sequenced packet, so process it
				svs.qh_stats.packets++;
				good = true;
				cl->qh_send_message = true;	// reply at end of frame
				if (cl->state != CS_ZOMBIE)
				{
					SV_ExecuteClientMessage(cl);
				}
			}
			break;
		}

		if (i != MAX_CLIENTS_QHW)
		{
			continue;
		}

		// packet is not from a known client
		//	Con_Printf ("%s:sequenced packet without connection\n"
		// ,SOCK_AdrToString(net_from));
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
void SV_CheckTimeouts(void)
{
	int i;
	client_t* cl;

	int droptime = realtime * 1000 - timeout->value * 1000;

	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if ((cl->state == CS_CONNECTED || cl->state == CS_ACTIVE) &&
			cl->netchan.lastReceived < droptime)
		{
			SV_BroadcastPrintf(PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient(cl);
			cl->state = CS_FREE;	// don't bother with zombie state
		}
		if (cl->state == CS_ZOMBIE &&
			realtime - cl->qh_connection_started > zombietime->value)
		{
			cl->state = CS_FREE;	// can now be reused
		}
	}
}

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void SV_GetConsoleCommands(void)
{
	char* cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput();
		if (!cmd)
		{
			break;
		}
		Cbuf_AddText(cmd);
	}
}

/*
===================
SV_CheckVars

===================
*/
void SV_CheckVars(void)
{
	static char* pw, * spw;
	int v;

	if (password->string == pw && spectator_password->string == spw)
	{
		return;
	}
	pw = password->string;
	spw = spectator_password->string;

	v = 0;
	if (pw && pw[0] && String::Cmp(pw, "none"))
	{
		v |= 1;
	}
	if (spw && spw[0] && String::Cmp(spw, "none"))
	{
		v |= 2;
	}

	Con_Printf("Updated needpass.\n");
	if (!v)
	{
		Info_SetValueForKey(svs.qh_info, "needpass", "", MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}
	else
	{
		Info_SetValueForKey(svs.qh_info, "needpass", va("%i",v), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}
}

/*
==================
SV_Frame

==================
*/
void SV_Frame(float time)
{
	try
	{
		static double start, end;

		start = Sys_DoubleTime();
		svs.qh_stats.idle += start - end;

// keep the random time dependent
		rand();

// decide the simulation time
		realtime += time;
		sv.qh_time += time;

		for (sysEvent_t ev = Sys_SharedGetEvent(); ev.evType; ev = Sys_SharedGetEvent())
		{
			switch (ev.evType)
			{
			case SE_CONSOLE:
				Cbuf_AddText((char*)ev.evPtr);
				Cbuf_AddText("\n");
				break;
			default:
				break;
			}

			// free any block data
			if (ev.evPtr)
			{
				Mem_Free(ev.evPtr);
			}
		}

// check timeouts
		SV_CheckTimeouts();

// toggle the log buffer if full
		SV_CheckLog();

// move autonomous things around if enough time has passed
		SVQH_RunPhysicsForTime(realtime);

// get packets
		SV_ReadPackets();

// check for commands typed to the host
		SV_GetConsoleCommands();

// process console commands
		Cbuf_Execute();

		SV_CheckVars();

// send messages back to the clients that had packets read this frame
		SV_SendClientMessages();

// send a heartbeat to the master if needed
		Master_Heartbeat();

// collect timing statistics
		end = Sys_DoubleTime();
		svs.qh_stats.active += end - start;
		if (++svs.qh_stats.count == STATFRAMES)
		{
			svs.qh_stats.latched_active = svs.qh_stats.active;
			svs.qh_stats.latched_idle = svs.qh_stats.idle;
			svs.qh_stats.latched_packets = svs.qh_stats.packets;
			svs.qh_stats.active = 0;
			svs.qh_stats.idle = 0;
			svs.qh_stats.packets = 0;
			svs.qh_stats.count = 0;
		}
	}
	catch (DropException& e)
	{
		SV_Error("%s", e.What());
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
===============
SV_InitLocal
===============
*/
void SV_InitLocal(void)
{
	int i;
	
	SV_InitOperatorCommands();
	SV_UserInit();

	rcon_password = Cvar_Get("rcon_password", "", 0);	// password for remote server commands
	password = Cvar_Get("password", "", 0);	// password for entering the game
	spectator_password = Cvar_Get("spectator_password", "", 0);	// password for entering as a sepctator

	fraglimit = Cvar_Get("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = Cvar_Get("timelimit", "0", CVAR_SERVERINFO);
	svqh_teamplay = Cvar_Get("teamplay", "0", CVAR_SERVERINFO);
	samelevel = Cvar_Get("samelevel", "0", CVAR_SERVERINFO);
	sv_maxclients = Cvar_Get("maxclients", "8", CVAR_SERVERINFO);
	maxspectators = Cvar_Get("maxspectators", "8", CVAR_SERVERINFO);
	skill = Cvar_Get("skill", "1", 0);						// 0 - 3
	svqh_deathmatch = Cvar_Get("deathmatch", "1", CVAR_SERVERINFO);			// 0, 1, or 2
	svqh_coop = Cvar_Get("coop", "0", CVAR_SERVERINFO);			// 0, 1, or 2
	randomclass = Cvar_Get("randomclass", "0", CVAR_SERVERINFO);
	damageScale = Cvar_Get("damagescale", "1.0", CVAR_SERVERINFO);
	shyRespawn = Cvar_Get("shyRespawn", "0", CVAR_SERVERINFO);
	spartanPrint = Cvar_Get("spartanPrint", "1.0", CVAR_SERVERINFO);
	meleeDamScale = Cvar_Get("meleeDamScale", "0.66666", CVAR_SERVERINFO);
	manaScale = Cvar_Get("manascale", "1.0", CVAR_SERVERINFO);
	tomeMode = Cvar_Get("tomemode", "0", CVAR_SERVERINFO);
	tomeRespawn = Cvar_Get("tomerespawn", "0", CVAR_SERVERINFO);
	w2Respawn = Cvar_Get("w2respawn", "0", CVAR_SERVERINFO);
	altRespawn = Cvar_Get("altrespawn", "0", CVAR_SERVERINFO);
	fixedLevel = Cvar_Get("fixedlevel", "0", CVAR_SERVERINFO);
	autoItems = Cvar_Get("autoitems", "0", CVAR_SERVERINFO);
	dmMode = Cvar_Get("dmmode", "0", CVAR_SERVERINFO);
	easyFourth = Cvar_Get("easyfourth", "0", CVAR_SERVERINFO);
	patternRunner = Cvar_Get("patternrunner", "0", CVAR_SERVERINFO);
	spawn = Cvar_Get("spawn", "0", CVAR_SERVERINFO);
	sv_hostname = Cvar_Get("hostname", "unnamed", CVAR_SERVERINFO);
	noexit = Cvar_Get("noexit", "0", CVAR_SERVERINFO);

	timeout = Cvar_Get("timeout", "65", 0);		// seconds without any message
	zombietime = Cvar_Get("zombietime", "2", 0);	// seconds to sink messages
	// after disconnect

	SVQH_RegisterPhysicsCvars();

	svqh_aim = Cvar_Get("sv_aim", "0.93", 0);

	filterban = Cvar_Get("filterban", "1", 0);

	allow_download = Cvar_Get("allow_download", "1", 0);
	allow_download_skins = Cvar_Get("allow_download_skins", "1", 0);
	allow_download_models = Cvar_Get("allow_download_models", "1", 0);
	allow_download_sounds = Cvar_Get("allow_download_sounds", "1", 0);
	allow_download_maps = Cvar_Get("allow_download_maps", "1", 0);

	svqh_highchars = Cvar_Get("sv_highchars", "1", 0);

	sv_phs = Cvar_Get("sv_phs", "1", 0);
	sv_namedistance = Cvar_Get("sv_namedistance", "600", 0);

	sv_ce_scale = Cvar_Get("sv_ce_scale", "1", CVAR_ARCHIVE);
	sv_ce_max_size = Cvar_Get("sv_ce_max_size", "0", CVAR_ARCHIVE);

	Cmd_AddCommand("addip", SV_AddIP_f);
	Cmd_AddCommand("removeip", SV_RemoveIP_f);
	Cmd_AddCommand("listip", SV_ListIP_f);
	Cmd_AddCommand("writeip", SV_WriteIP_f);

	for (i = 0; i < MAX_MODELS_H2; i++)
		sprintf(localmodels[i], "*%i", i);

	Info_SetValueForKey(svs.qh_info, "*version", va("%4.2f", VERSION), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);

	svs.clients = new client_t[MAX_CLIENTS_QHW];
	Com_Memset(svs.clients, 0, sizeof(client_t) * MAX_CLIENTS_QHW);

	// init fraglog stuff
	svs.qh_logsequence = 1;
	svs.qh_logtime = realtime;
	svs.qh_log[0].InitOOB(svs.qh_log_buf[0], sizeof(svs.qh_log_buf[0]));
	svs.qh_log[0].allowoverflow = true;
	svs.qh_log[1].InitOOB(svs.qh_log_buf[1], sizeof(svs.qh_log_buf[1]));
	svs.qh_log[1].allowoverflow = true;
}


//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define HEARTBEAT_SECONDS   300
void Master_Heartbeat(void)
{
	char string[2048];
	int active;
	int i;

	if (realtime - svs.qh_last_heartbeat < HEARTBEAT_SECONDS)
	{
		return;		// not time to send yet

	}
	svs.qh_last_heartbeat = realtime;

	//
	// count active users
	//
	active = 0;
	for (i = 0; i < MAX_CLIENTS_QHW; i++)
		if (svs.clients[i].state == CS_CONNECTED ||
			svs.clients[i].state == CS_ACTIVE)
		{
			active++;
		}

	svs.qh_heartbeat_sequence++;
	sprintf(string, "%c\n%i\n%i\n", S2M_HEARTBEAT,
		svs.qh_heartbeat_sequence, active);


	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			Con_Printf("Sending heartbeat to %s\n", SOCK_AdrToString(master_adr[i]));
			NET_SendPacket(String::Length(string), string, master_adr[i]);
		}

#ifndef _DEBUG
	// send to id master
	NET_SendPacket(String::Length(string), string, idmaster_adr);
#endif
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown(void)
{
	char string[2048];
	int i;

	sprintf(string, "%c\n", S2M_SHUTDOWN);

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			Con_Printf("Sending heartbeat to %s\n", SOCK_AdrToString(master_adr[i]));
			NET_SendPacket(String::Length(string), string, master_adr[i]);
		}

	// send to id master
#ifndef _DEBUG
	NET_SendPacket(String::Length(string), string, idmaster_adr);
#endif
}

/*
=================
SV_ExtractFromUserinfo

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_ExtractFromUserinfo(client_t* cl)
{
	const char* val;
	char* p, * q;
	int i;
	client_t* client;
	int dupc = 1;
	char newname[80];


	// name for C code
	val = Info_ValueForKey(cl->userinfo, "name");

	// trim user name
	String::NCpy(newname, val, sizeof(newname) - 1);
	newname[sizeof(newname) - 1] = 0;

	for (p = newname; *p == ' ' && *p; p++)
		;
	if (p != newname && *p)
	{
		for (q = newname; *p; *q++ = *p++)
			;
		*q = 0;
	}
	for (p = newname + String::Length(newname) - 1; p != newname && *p == ' '; p--)
		;
	p[1] = 0;

	if (String::Cmp(val, newname))
	{
		Info_SetValueForKey(cl->userinfo, "name", newname, HWMAX_INFO_STRING, 64, 64, !svqh_highchars->value);
		val = Info_ValueForKey(cl->userinfo, "name");
	}

	if (!val[0] || !String::ICmp(val, "console"))
	{
		Info_SetValueForKey(cl->userinfo, "name", "unnamed", HWMAX_INFO_STRING, 64, 64, !svqh_highchars->value);
		val = Info_ValueForKey(cl->userinfo, "name");
	}

	// check to see if another user by the same name exists
	while (1)
	{
		for (i = 0, client = svs.clients; i < MAX_CLIENTS_QHW; i++, client++)
		{
			if (client->state != CS_ACTIVE || client == cl)
			{
				continue;
			}
			if (!String::ICmp(client->name, val))
			{
				break;
			}
		}
		if (i != MAX_CLIENTS_QHW)
		{	// dup name
			char tmp[80];
			String::NCpyZ(tmp, val, sizeof(tmp));
			if (String::Length(val) > (int)sizeof(cl->name) - 1)
			{
				tmp[sizeof(cl->name) - 4] = 0;
			}
			p = tmp;

			if (tmp[0] == '(')
			{
				if (tmp[2] == ')')
				{
					p = tmp + 3;
				}
				else if (tmp[3] == ')')
				{
					p = tmp + 4;
				}
			}

			sprintf(newname, "(%d)%-0.40s", dupc++, p);
			Info_SetValueForKey(cl->userinfo, "name", newname, HWMAX_INFO_STRING, 64, 64, !svqh_highchars->value);
			val = Info_ValueForKey(cl->userinfo, "name");
		}
		else
		{
			break;
		}
	}

	String::NCpy(cl->name, val, sizeof(cl->name) - 1);

	// rate command
	val = Info_ValueForKey(cl->userinfo, "rate");
	if (String::Length(val))
	{
		i = String::Atoi(val);
		if (i < 500)
		{
			i = 500;
		}
		if (i > 10000)
		{
			i = 10000;
		}
		cl->netchan.rate = 1.0 / i;
	}

	// playerclass command
	val = Info_ValueForKey(cl->userinfo, "playerclass");
	if (String::Length(val))
	{
		i = String::Atoi(val);
		if (i > CLASS_DEMON && dmMode->value != DM_SIEGE)
		{
			i = CLASS_PALADIN;
		}
		if (i < 0 || i > MAX_PLAYER_CLASS || (!cl->hw_portals && i == CLASS_DEMON))
		{
			i = 0;
		}
		cl->hw_next_playerclass =  i;
		cl->qh_edict->SetNextPlayerClass(i);

		if (cl->qh_edict->GetHealth() > 0)
		{
			sprintf(newname,"%d",cl->h2_playerclass);
			Info_SetValueForKey(cl->userinfo, "playerclass", newname, HWMAX_INFO_STRING, 64, 64, !svqh_highchars->value);
		}
	}

	// msg command
	val = Info_ValueForKey(cl->userinfo, "msg");
	if (String::Length(val))
	{
		cl->messagelevel = String::Atoi(val);
	}

}


//============================================================================

/*
====================
SV_InitNet
====================
*/
void SV_InitNet(void)
{
	int p;

	sv_net_port = PORT_SERVER;
	p = COM_CheckParm("-port");
	if (p && p < COM_Argc())
	{
		sv_net_port = String::Atoi(COM_Argv(p + 1));
		Con_Printf("Port: %i\n", sv_net_port);
	}
	NET_Init(sv_net_port);

	Netchan_Init();

	// heartbeats will allways be sent to the id master
	svs.qh_last_heartbeat = -99999;		// send immediately

	SOCK_StringToAdr("208.135.137.23", &idmaster_adr, 26900);
}


/*
====================
SV_Init
====================
*/
void SV_Init(quakeparms_t* parms)
{
	try
	{
		GGameType = GAME_Hexen2 | GAME_HexenWorld;
		Sys_SetHomePathSuffix("jlhexen2");
		Log::addListener(&MainLog);

		COM_InitArgv2(parms->argc, parms->argv);
//	COM_AddParm ("-game");
//	COM_AddParm ("hw");

		if (COM_CheckParm("-minmemory"))
		{
			parms->memsize = MINIMUM_MEMORY;
		}

		host_parms = *parms;

		if (parms->memsize < MINIMUM_MEMORY)
		{
			SV_Error("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);
		}

		Memory_Init(parms->membase, parms->memsize);
		Cbuf_Init();
		Cmd_Init();
		Cvar_Init();

		com_dedicated = Cvar_Get("dedicated", "1", CVAR_ROM);

		COM_Init(parms->basedir);

		PR_Init();

		SV_InitNet();

		SV_InitLocal();
		Sys_Init();
		PMQH_Init();

		Hunk_AllocName(0, "-HOST_HUNKLEVEL-");
		host_hunklevel = Hunk_LowMark();

		Cbuf_InsertText("exec server.cfg\n");

		host_initialized = true;

		Con_Printf("Exe: "__TIME__ " "__DATE__ "\n");
		Con_Printf("%4.1f megabyte heap\n",parms->memsize / (1024 * 1024.0));
		Con_Printf("======== HexenWorld Initialized ========\n");

// process command line arguments
		Cbuf_AddLateCommands();
		Cbuf_Execute();

// if a map wasn't specified on the command line, spawn start.map

		if (sv.state == SS_DEAD)
		{
			Cmd_ExecuteString("map demo1");
		}
		if (sv.state == SS_DEAD)
		{
			SV_Error("Couldn't spawn a server");
		}
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}
