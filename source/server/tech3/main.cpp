//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "local.h"
#include "../../common/file_formats/tag.h"
#include "../wolfmp/local.h"
#include "../et/local.h"
#include "../public.h"

#define HEARTBEAT_MSEC      300 * 1000
#define Q3HEARTBEAT_GAME    "QuakeArena-1"
#define WSMHEARTBEAT_GAME   "Wolfenstein-1"
#define WMHEARTBEAT_DEAD    "WolfFlatline-1"
#define ETHEARTBEAT_GAME    "EnemyTerritory-1"
#define ETHEARTBEAT_DEAD    "ETFlatline-1"			// NERVE - SMF

Cvar* svt3_gametype;
Cvar* svt3_pure;
Cvar* svt3_padPackets;		// add nop bytes to messages
Cvar* svt3_maxRate;
Cvar* svt3_dl_maxRate;
Cvar* svt3_mapname;
Cvar* svt3_timeout;			// seconds without any message
Cvar* svt3_zombietime;		// seconds to sink messages after disconnect
Cvar* svt3_allowDownload;
Cvar* svet_wwwFallbackURL;	// URL to send to if an http/ftp fails or is refused client side
Cvar* svet_wwwDownload;		// server does a www dl redirect
Cvar* svet_wwwBaseURL;	// base URL for redirect
// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you loose the ability to control the download usage
Cvar* svet_wwwDlDisconnected;
Cvar* svt3_lanForceRate;		// dedicated 1 (LAN) server forces local client rates to 99999 (bug #491)
Cvar* svwm_showAverageBPS;		// NERVE - SMF - net debugging
Cvar* svet_tempbanmessage;
Cvar* svwm_onlyVisibleClients;
Cvar* svq3_strictAuth;
Cvar* svet_fullmsg;
Cvar* svt3_reconnectlimit;		// minimum seconds between connect messages
Cvar* svt3_minPing;
Cvar* svt3_maxPing;
Cvar* svt3_privatePassword;		// password for the privateClient slots
Cvar* svt3_privateClients;		// number of clients reserved for password
Cvar* svt3_floodProtect;
Cvar* svt3_reloading;
Cvar* svt3_master[Q3MAX_MASTER_SERVERS];		// master server ip address
Cvar* svt3_gameskill;
Cvar* svt3_allowAnonymous;
Cvar* svt3_friendlyFire;			// NERVE - SMF
Cvar* svt3_maxlives;				// NERVE - SMF
Cvar* svwm_tourney;				// NERVE - SMF
Cvar* svet_needpass;
Cvar* svt3_rconPassword;			// password for remote server commands
Cvar* svt3_fps;					// time rate for running non-clients
Cvar* svt3_killserver;			// menu system can set to 1 to shut server down

//	Converts newlines to "\n" so a line prints nicer
static const char* SVT3_ExpandNewlines(const char* in)
{
	static char string[1024];
	int l = 0;
	while (*in && l < (int)sizeof(string) - 3)
	{
		if (*in == '\n')
		{
			string[l++] = '\\';
			string[l++] = 'n';
		}
		else
		{
			// NERVE - SMF - HACK - strip out localization tokens before string command is displayed in syscon window
			if (GGameType & (GAME_WolfMP | GAME_ET) &&
				(!String::NCmp(in, "[lon]", 5) || !String::NCmp(in, "[lof]", 5)))
			{
				in += 5;
				continue;
			}

			string[l++] = *in;
		}
		in++;
	}
	string[l] = 0;

	return string;
}

//	The given command will be transmitted to the client, and is guaranteed to
// not have future snapshot_t executed before it is executed
void SVT3_AddServerCommand(client_t* client, const char* cmd)
{
	client->q3_reliableSequence++;
	// if we would be losing an old command that hasn't been acknowledged,
	// we must drop the connection
	// we check == instead of >= so a broadcast print added by SVT3_DropClient()
	// doesn't cause a recursive drop client
	int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
	if (client->q3_reliableSequence - client->q3_reliableAcknowledge == maxReliableCommands + 1)
	{
		common->Printf("===== pending server commands =====\n");
		int i;
		for (i = client->q3_reliableAcknowledge + 1; i <= client->q3_reliableSequence; i++)
		{
			common->Printf("cmd %5d: %s\n", i, SVT3_GetReliableCommand(client, i & (maxReliableCommands - 1)));
		}
		common->Printf("cmd %5d: %s\n", i, cmd);
		SVT3_DropClient(client, "Server command overflow");
		return;
	}
	int index = client->q3_reliableSequence & (maxReliableCommands - 1);
	SVT3_AddReliableCommand(client, index, cmd);
}

//	Sends a reliable command string to be interpreted by
// the client game module: "cp", "print", "chat", etc
// A NULL client will broadcast to all clients
void SVT3_SendServerCommand(client_t* cl, const char* fmt, ...)
{
	va_list argptr;
	byte message[MAX_MSGLEN];
	va_start(argptr,fmt);
	Q_vsnprintf((char*)message, sizeof(message), fmt, argptr);
	va_end(argptr);

	// do not forward server command messages that would be too big to clients
	// ( q3infoboom / q3msgboom stuff )
	if (GGameType & (GAME_WolfMP | GAME_ET) && String::Length((char*)message) > 1022)
	{
		return;
	}

	if (cl != NULL)
	{
		SVT3_AddServerCommand(cl, (char*)message);
		return;
	}

	// hack to echo broadcast prints to console
	if (com_dedicated->integer && !String::NCmp((char*)message, "print", 5))
	{
		common->Printf("broadcast: %s\n", SVT3_ExpandNewlines((char*)message));
	}

	// send the data to all relevent clients
	client_t* client = svs.clients;
	for (int j = 0; j < sv_maxclients->integer; j++, client++)
	{
		if (client->state < CS_PRIMED)
		{
			continue;
		}
		// Ridah, don't need to send messages to AI
		if (client->q3_entity && client->q3_entity->GetSvFlagCastAI())
		{
			continue;
		}
		if (GGameType & GAME_ET && client->q3_entity && client->q3_entity->GetSvFlags() & Q3SVF_BOT)
		{
			continue;
		}
		SVT3_AddServerCommand(client, (char*)message);
	}
}

int SVET_LoadTag(const char* mod_name)
{
	for (int i = 0; i < sv.et_num_tagheaders; i++)
	{
		if (!String::ICmp(mod_name, sv.et_tagHeadersExt[i].filename))
		{
			return i + 1;
		}
	}

	unsigned char* buffer;
	FS_ReadFile(mod_name, (void**)&buffer);

	if (!buffer)
	{
		return 0;
	}

	tagHeader_t* pinmodel = (tagHeader_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != TAG_VERSION)
	{
		common->Printf(S_COLOR_YELLOW "WARNING: SVET_LoadTag: %s has wrong version (%i should be %i)\n", mod_name, version, TAG_VERSION);
		FS_FreeFile(buffer);
		return 0;
	}

	if (sv.et_num_tagheaders >= MAX_TAG_FILES_ET)
	{
		common->Error("MAX_TAG_FILES_ET reached\n");

		FS_FreeFile(buffer);
		return 0;
	}

#define LL(x) x = LittleLong(x)
	LL(pinmodel->ident);
	LL(pinmodel->numTags);
	LL(pinmodel->ofsEnd);
	LL(pinmodel->version);
#undef LL

	String::NCpyZ(sv.et_tagHeadersExt[sv.et_num_tagheaders].filename, mod_name, MAX_QPATH);
	sv.et_tagHeadersExt[sv.et_num_tagheaders].start = sv.et_num_tags;
	sv.et_tagHeadersExt[sv.et_num_tagheaders].count = pinmodel->numTags;

	if (sv.et_num_tags + pinmodel->numTags >= MAX_SERVER_TAGS_ET)
	{
		common->Error("MAX_SERVER_TAGS_ET reached\n");

		FS_FreeFile(buffer);
		return 0;
	}

	// swap all the tags
	md3Tag_t* tag = &sv.et_tags[sv.et_num_tags];
	sv.et_num_tags += pinmodel->numTags;
	md3Tag_t* readTag = (md3Tag_t*)(buffer + sizeof(tagHeader_t));
	for (int i = 0; i < pinmodel->numTags; i++, tag++, readTag++)
	{
		for (int j = 0; j < 3; j++)
		{
			tag->origin[j] = LittleFloat(readTag->origin[j]);
			tag->axis[0][j] = LittleFloat(readTag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(readTag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(readTag->axis[2][j]);
		}
		String::NCpyZ(tag->name, readTag->name, 64);
	}

	FS_FreeFile(buffer);
	return ++sv.et_num_tagheaders;
}

//	Updates the cl->ping variables
static void SVT3_CalcPings()
{
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t* cl = &svs.clients[i];
		if (cl->state != CS_ACTIVE)
		{
			cl->ping = 999;
			continue;
		}
		if (!cl->q3_entity)
		{
			cl->ping = 999;
			continue;
		}
		if (cl->q3_entity->GetSvFlags() & Q3SVF_BOT)
		{
			cl->ping = 0;
			continue;
		}

		int total = 0;
		int count = 0;
		for (int j = 0; j < PACKET_BACKUP_Q3; j++)
		{
			if (cl->q3_frames[j].messageAcked <= 0)
			{
				continue;
			}
			int delta = cl->q3_frames[j].messageAcked - cl->q3_frames[j].messageSent;
			count++;
			total += delta;
		}
		if (!count)
		{
			cl->ping = 999;
		}
		else
		{
			cl->ping = total / count;
			if (cl->ping > 999)
			{
				cl->ping = 999;
			}
		}

		// let the game dll know about the ping
		idPlayerState3* ps = SVT3_GameClientNum(i);
		ps->SetPing(cl->ping);
	}
}

//	If a packet has not been received from a client for timeout->integer
// seconds, drop the conneciton.  Server time is used instead of
// realtime to avoid dropping the local client while debugging.
//	When a client is normally dropped, the client_t goes into a zombie state
// for a few seconds to make sure any final reliable message gets resent
// if necessary
static void SVT3_CheckTimeouts()
{
	int droppoint = svs.q3_time - 1000 * svt3_timeout->integer;
	int zombiepoint = svs.q3_time - 1000 * svt3_zombietime->integer;

	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->q3_lastPacketTime > svs.q3_time)
		{
			cl->q3_lastPacketTime = svs.q3_time;
		}

		if (cl->state == CS_ZOMBIE &&
			cl->q3_lastPacketTime < zombiepoint)
		{
			// using the client id cause the cl->name is empty at this point
			common->DPrintf("Going from CS_ZOMBIE to CS_FREE for client %d\n", i);
			cl->state = CS_FREE;	// can now be reused
			continue;
		}
		// Ridah, AI's don't time out
		if (!(GGameType & GAME_WolfSP) || (cl->q3_entity && !cl->q3_entity->GetSvFlagCastAI()))
		{
			if (cl->state >= CS_CONNECTED && cl->q3_lastPacketTime < droppoint)
			{
				// wait several frames so a debugger session doesn't
				// cause a timeout
				if (++cl->q3_timeoutCount > 5)
				{
					SVT3_DropClient(cl, "timed out");
					cl->state = CS_FREE;	// don't bother with zombie state
				}
			}
			else
			{
				cl->q3_timeoutCount = 0;
			}
		}
	}
}

static bool SVT3_CheckPaused()
{
	if (!cl_paused->integer)
	{
		return false;
	}

	// only pause if there is just a single client connected
	int count = 0;
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state >= CS_CONNECTED && cl->netchan.remoteAddress.type != NA_BOT)
		{
			count++;
		}
	}

	if (count > 1)
	{
		// don't pause
		if (sv_paused->integer)
		{
			Cvar_Set("sv_paused", "0");
		}
		return false;
	}

	if (!sv_paused->integer)
	{
		Cvar_Set("sv_paused", "1");
	}
	return true;
}

//returns true if valid challenge
//returns false if m4d h4x0rz
static bool SVET_VerifyChallenge(const char* challenge)
{
	if (!challenge)
	{
		return false;
	}

	int j = String::Length(challenge);
	if (j > 64)
	{
		return false;
	}
	for (int i = 0; i < j; i++)
	{
		if (challenge[i] == '\\' ||
			challenge[i] == '/' ||
			challenge[i] == '%' ||
			challenge[i] == ';' ||
			challenge[i] == '"' ||
			challenge[i] < 32 ||	// non-ascii
			challenge[i] > 126	// non-ascii
			)
		{
			return false;
		}
	}
	return true;
}

//	NERVE - SMF - Send serverinfo cvars, etc to master servers when
// game complete. Useful for tracking global player stats.
static void SVT3C_GameCompleteStatus(netadr_t from)
{
	char player[1024];
	char status[MAX_MSGLEN_WOLF];
	int i;
	client_t* cl;
	int statusLength;
	int playerLength;
	char infostring[MAX_INFO_STRING_Q3];

	// ignore if we are in single player
	if (GGameType & GAME_WolfMP && Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER)
	{
		return;
	}
	if (GGameType & GAME_ET && SVET_GameIsSinglePlayer())
	{
		return;
	}

	//bani - bugtraq 12534
	if (GGameType & GAME_ET && !SVET_VerifyChallenge(Cmd_Argv(1)))
	{
		return;
	}

	String::Cpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	status[0] = 0;
	statusLength = 0;

	for (i = 0; i < sv_maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED)
		{
			idPlayerState3* ps = SVT3_GameClientNum(i);
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n", ps->GetPersistantScore(), cl->ping, cl->name);
			playerLength = String::Length(player);
			if (statusLength + playerLength >= (int)sizeof(status))
			{
				break;		// can't hold any more
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "gameCompleteStatus\n%s\n%s", infostring, status);
}

//	Responds with all the info that qplug or qspy can see about the server
// and all connected players.  Used for getting detailed information after
// the simple info query.
static void SVT3C_Status(netadr_t from)
{
	// ignore if we are in single player
	if (!(GGameType & GAME_ET) && Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER)
	{
		return;
	}
	if (GGameType & GAME_ET && SVET_GameIsSinglePlayer())
	{
		return;
	}

	//bani - bugtraq 12534
	if (GGameType & GAME_ET && !SVET_VerifyChallenge(Cmd_Argv(1)))
	{
		return;
	}

	char infostring[MAX_INFO_STRING_Q3];
	String::Cpy(infostring, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));

	// echo back the parameter to status. so master servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	char status[MAX_MSGLEN];
	status[0] = 0;
	int statusLength = 0;

	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t* cl = &svs.clients[i];
		if (cl->state >= CS_CONNECTED)
		{
			idPlayerState3* ps = SVT3_GameClientNum(i);
			char player[1024];
			String::Sprintf(player, sizeof(player), "%i %i \"%s\"\n",
				ps->GetPersistantScore(), cl->ping, cl->name);
			int playerLength = String::Length(player);
			if (statusLength + playerLength >= (GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF))
			{
				break;		// can't hold any more
			}
			String::Cpy(status + statusLength, player);
			statusLength += playerLength;
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "statusResponse\n%s\n%s", infostring, status);
}

//	Responds with a short info message that should be enough to determine
// if a user is interested in a server to do a full status
static void SVT3C_Info(netadr_t from)
{
	// ignore if we are in single player
	if (!(GGameType & GAME_ET) && Cvar_VariableValue("g_gametype") == Q3GT_SINGLE_PLAYER)
	{
		return;
	}
	if (GGameType & GAME_Quake3 && Cvar_VariableValue("ui_singlePlayerActive"))
	{
		return;
	}
	if (GGameType & GAME_ET && SVET_GameIsSinglePlayer())
	{
		return;
	}

	//bani - bugtraq 12534
	if (GGameType & GAME_ET && !SVET_VerifyChallenge(Cmd_Argv(1)))
	{
		return;
	}

	// don't count privateclients
	int count = 0;
	for (int i = svt3_privateClients->integer; i < sv_maxclients->integer; i++)
	{
		if (svs.clients[i].state >= CS_CONNECTED)
		{
			count++;
		}
	}

	char infostring[MAX_INFO_STRING_Q3];
	infostring[0] = 0;

	// echo back the parameter to status. so servers can use it as a challenge
	// to prevent timed spoofed reply packets that add ghost servers
	Info_SetValueForKey(infostring, "challenge", Cmd_Argv(1), MAX_INFO_STRING_Q3);

	if (GGameType & GAME_Quake3)
	{
		Info_SetValueForKey(infostring, "protocol", va("%i", Q3PROTOCOL_VERSION), MAX_INFO_STRING_Q3);
	}
	else if (GGameType & GAME_WolfSP)
	{
		Info_SetValueForKey(infostring, "protocol", va("%i", WSPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
	}
	else if (GGameType & GAME_WolfMP)
	{
		Info_SetValueForKey(infostring, "protocol", va("%i", WMPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
	}
	else
	{
		Info_SetValueForKey(infostring, "protocol", va("%i", ETPROTOCOL_VERSION), MAX_INFO_STRING_Q3);
	}
	Info_SetValueForKey(infostring, "hostname", sv_hostname->string, MAX_INFO_STRING_Q3);
	if (GGameType & GAME_ET)
	{
		Info_SetValueForKey(infostring, "serverload", va("%i", svs.et_serverLoad), MAX_INFO_STRING_Q3);
	}
	Info_SetValueForKey(infostring, "mapname", svt3_mapname->string, MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "clients", va("%i", count), MAX_INFO_STRING_Q3);
	Info_SetValueForKey(infostring, "sv_maxclients",
		va("%i", sv_maxclients->integer - svt3_privateClients->integer), MAX_INFO_STRING_Q3);
	if (GGameType & GAME_ET)
	{
		Info_SetValueForKey(infostring, "gametype", Cvar_VariableString("g_gametype"), MAX_INFO_STRING_Q3);
	}
	else
	{
		Info_SetValueForKey(infostring, "gametype", va("%i", svt3_gametype->integer), MAX_INFO_STRING_Q3);
	}
	Info_SetValueForKey(infostring, "pure", va("%i", svt3_pure->integer), MAX_INFO_STRING_Q3);

	if (svt3_minPing->integer)
	{
		Info_SetValueForKey(infostring, "minPing", va("%i", svt3_minPing->integer), MAX_INFO_STRING_Q3);
	}
	if (svt3_maxPing->integer)
	{
		Info_SetValueForKey(infostring, "maxPing", va("%i", svt3_maxPing->integer), MAX_INFO_STRING_Q3);
	}
	const char* gamedir = Cvar_VariableString("fs_game");
	if (*gamedir)
	{
		Info_SetValueForKey(infostring, "game", gamedir, MAX_INFO_STRING_Q3);
	}
	if (!(GGameType & GAME_Quake3))
	{
		Info_SetValueForKey(infostring, "sv_allowAnonymous", va("%i", svt3_allowAnonymous->integer), MAX_INFO_STRING_Q3);
	}

	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		// Rafael gameskill
		Info_SetValueForKey(infostring, "gameskill", va("%i", svt3_gameskill->integer), MAX_INFO_STRING_Q3);
		// done
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		Info_SetValueForKey(infostring, "friendlyFire", va("%i", svt3_friendlyFire->integer), MAX_INFO_STRING_Q3);			// NERVE - SMF
		Info_SetValueForKey(infostring, "maxlives", va("%i", svt3_maxlives->integer ? 1 : 0), MAX_INFO_STRING_Q3);			// NERVE - SMF
	}
	if (GGameType & GAME_WolfMP)
	{
		Info_SetValueForKey(infostring, "tourney", va("%i", svwm_tourney->integer), MAX_INFO_STRING_Q3);					// NERVE - SMF
		Info_SetValueForKey(infostring, "gamename", WMGAMENAME_STRING, MAX_INFO_STRING_Q3);									// Arnout: to be able to filter out Quake servers
	}
	if (GGameType & GAME_ET)
	{
		Info_SetValueForKey(infostring, "needpass", va("%i", svet_needpass->integer ? 1 : 0), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(infostring, "gamename", ETGAMENAME_STRING, MAX_INFO_STRING_Q3);									// Arnout: to be able to filter out Quake servers
	}

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		// TTimo
		const char* antilag = Cvar_VariableString("g_antilag");
		if (antilag)
		{
			Info_SetValueForKey(infostring, "g_antilag", antilag, MAX_INFO_STRING_Q3);
		}
	}

	if (GGameType & GAME_ET)
	{
		const char* weaprestrict = Cvar_VariableString("g_heavyWeaponRestriction");
		if (weaprestrict)
		{
			Info_SetValueForKey(infostring, "weaprestrict", weaprestrict, MAX_INFO_STRING_Q3);
		}

		const char* balancedteams = Cvar_VariableString("g_balancedteams");
		if (balancedteams)
		{
			Info_SetValueForKey(infostring, "balancedteams", balancedteams, MAX_INFO_STRING_Q3);
		}
	}

	NET_OutOfBandPrint(NS_SERVER, from, "infoResponse\n%s", infostring);
}

static void SVT3_FlushRedirect(char* outputbuf)
{
	NET_OutOfBandPrint(NS_SERVER, svs.redirectAddress, "print\n%s", outputbuf);
}

//	An rcon packet arrived from the network.
//	Shift down the remaining args
//	Redirect all printfs
static void SVT3C_RemoteCommand(netadr_t from, QMsg* msg)
{
	static unsigned int lasttime = 0;

	unsigned int time = Com_Milliseconds();
	if (time < (lasttime + 500))
	{
		return;
	}
	lasttime = time;

	bool valid;
	if (!String::Length(svt3_rconPassword->string) ||
		String::Cmp(Cmd_Argv(1), svt3_rconPassword->string))
	{
		valid = false;
		common->Printf("Bad rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}
	else
	{
		valid = true;
		common->Printf("Rcon from %s:\n%s\n", SOCK_AdrToString(from), Cmd_Argv(2));
	}

	// start redirecting all print outputs to the packet
	svs.redirectAddress = from;
	// TTimo - scaled down to accumulate, but not overflow anything network wise, print wise etc.
	// (OOB messages are the bottleneck here)
	enum { SV_OUTPUTBUF_LENGTH = 1024 - 16 };
	char sv_outputbuf[SV_OUTPUTBUF_LENGTH];
	Com_BeginRedirect(sv_outputbuf, SV_OUTPUTBUF_LENGTH, SVT3_FlushRedirect);

	if (!String::Length(svt3_rconPassword->string))
	{
		common->Printf("No rconpassword set on the server.\n");
	}
	else if (!valid)
	{
		common->Printf("Bad rconpassword.\n");
	}
	else
	{
		char remaining[1024];
		remaining[0] = 0;

		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
		// get the command directly, "rcon <pass> <command>" to avoid quoting issues
		// extract the command by walking
		// since the cmd formatting can fuckup (amount of spaces), using a dumb step by step parsing
		char* cmd_aux = Cmd_Cmd();
		cmd_aux += 4;
		while (cmd_aux[0] == ' ')
		{
			cmd_aux++;
		}
		while (cmd_aux[0] && cmd_aux[0] != ' ')	// password
		{
			cmd_aux++;
		}
		while (cmd_aux[0] == ' ')
		{
			cmd_aux++;
		}

		String::Cat(remaining, sizeof(remaining), cmd_aux);

		Cmd_ExecuteString(remaining);
	}

	Com_EndRedirect();
}

//	A connectionless packet has four leading 0xff
// characters to distinguish it from a game channel.
// Clients that are in the game can still send
// connectionless packets.
static void SVT3_ConnectionlessPacket(netadr_t from, QMsg* msg)
{
	msg->BeginReadingOOB();
	msg->ReadLong();		// skip the -1 marker

	if (!(GGameType & GAME_WolfSP) && !String::NCmp("connect", (char*)&msg->_data[4], 7))
	{
		Huff_Decompress(msg, 12);
	}

	const char* s = msg->ReadStringLine();

	Cmd_TokenizeString(s);

	const char* c = Cmd_Argv(0);
	common->DPrintf("SV packet %s : %s\n", SOCK_AdrToString(from), c);

	if (!String::ICmp(c, "getstatus"))
	{
		SVT3C_Status(from);
	}
	else if (!String::ICmp(c, "getinfo"))
	{
		SVT3C_Info(from);
	}
	else if (!String::ICmp(c, "getchallenge"))
	{
		SVT3_GetChallenge(from);
	}
	else if (!String::ICmp(c, "connect"))
	{
		SVT3_DirectConnect(from);
	}
	else if (!(GGameType & GAME_ET) && !String::ICmp(c, "ipAuthorize"))
	{
		SVT3_AuthorizeIpPacket(from);
	}
	else if (!String::ICmp(c, "rcon"))
	{
		SVT3C_RemoteCommand(from, msg);
	}
	else if (!String::ICmp(c, "disconnect"))
	{
		// if a client starts up a local server, we may see some spurious
		// server disconnect messages when their new server sees our final
		// sequenced messages to the old client
	}
	else
	{
		common->DPrintf("bad connectionless packet from %s:\n%s\n",
			SOCK_AdrToString(from), s);
	}
}

/*
==============================================================================

MASTER SERVER FUNCTIONS

==============================================================================
*/

//	Send a message to the masters every few minutes to
// let it know we are alive, and log information.
// We will also have a heartbeat sent when a server
// changes from empty to non-empty, and full to non-full,
// but not on every player enter or exit.
static void SVT3_MasterHeartbeat(const char* hbname)
{
	static netadr_t adr[Q3MAX_MASTER_SERVERS];

	if (GGameType & GAME_ET && SVET_GameIsSinglePlayer())
	{
		return;		// no heartbeats for SP
	}

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2)
	{
		return;		// only dedicated servers send heartbeats
	}

	// if not time yet, don't send anything
	if (svs.q3_time < svs.q3_nextHeartbeatTime)
	{
		return;
	}
	svs.q3_nextHeartbeatTime = svs.q3_time + HEARTBEAT_MSEC;


	// send to group masters
	for (int i = 0; i < Q3MAX_MASTER_SERVERS; i++)
	{
		if (!svt3_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (svt3_master[i]->modified)
		{
			svt3_master[i]->modified = false;

			common->Printf("Resolving %s\n", svt3_master[i]->string);
			if (!SOCK_StringToAdr(svt3_master[i]->string, &adr[i], Q3PORT_MASTER))
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				common->Printf("Couldn't resolve address: %s\n", svt3_master[i]->string);
				Cvar_Set(svt3_master[i]->name, "");
				svt3_master[i]->modified = false;
				continue;
			}
			common->Printf("%s resolved to %s\n", svt3_master[i]->string, SOCK_AdrToString(adr[i]));
		}


		common->Printf("Sending heartbeat to %s\n", svt3_master[i]->string);
		// this command should be changed if the server info / status format
		// ever incompatably changes
		NET_OutOfBandPrint(NS_SERVER, adr[i], "heartbeat %s\n", hbname);
	}
}

//	NERVE - SMF - Sends gameCompleteStatus messages to all master servers
void SVT3_MasterGameCompleteStatus()
{
	static netadr_t adr[Q3MAX_MASTER_SERVERS];

	if (GGameType & GAME_ET && SVET_GameIsSinglePlayer())
	{
		return;		// no master game status for SP
	}

	// "dedicated 1" is for lan play, "dedicated 2" is for inet public play
	if (!com_dedicated || com_dedicated->integer != 2)
	{
		return;		// only dedicated servers send master game status
	}

	// send to group masters
	for (int i = 0; i < Q3MAX_MASTER_SERVERS; i++)
	{
		if (!svt3_master[i]->string[0])
		{
			continue;
		}

		// see if we haven't already resolved the name
		// resolving usually causes hitches on win95, so only
		// do it when needed
		if (svt3_master[i]->modified)
		{
			svt3_master[i]->modified = false;

			common->Printf("Resolving %s\n", svt3_master[i]->string);
			if (!SOCK_StringToAdr(svt3_master[i]->string, &adr[i], Q3PORT_MASTER))
			{
				// if the address failed to resolve, clear it
				// so we don't take repeated dns hits
				common->Printf("Couldn't resolve address: %s\n", svt3_master[i]->string);
				Cvar_Set(svt3_master[i]->name, "");
				svt3_master[i]->modified = false;
				continue;
			}
			common->Printf("%s resolved to %s\n", svt3_master[i]->string, SOCK_AdrToString(adr[i]));
		}

		common->Printf("Sending gameCompleteStatus to %s\n", svt3_master[i]->string);
		// this command should be changed if the server info / status format
		// ever incompatably changes
		SVT3C_GameCompleteStatus(adr[i]);
	}
}

//	Informs all masters that this server is going down
void SVT3_MasterShutdown()
{
	const char* hbname = GGameType & GAME_Quake3 ? Q3HEARTBEAT_GAME :
		GGameType & GAME_WolfSP ? WSMHEARTBEAT_GAME :
		GGameType & GAME_WolfMP ? WMHEARTBEAT_DEAD : ETHEARTBEAT_DEAD;		// NERVE - SMF - changed to flatline

	// send a hearbeat right now
	svs.q3_nextHeartbeatTime = -9999;
	SVT3_MasterHeartbeat(hbname);

	if (!(GGameType & (GAME_WolfMP | GAME_ET)))
	{
		// send it again to minimize chance of drops
		svs.q3_nextHeartbeatTime = -9999;
		SVT3_MasterHeartbeat(hbname);
	}

	// when the master tries to poll the server, it won't respond, so
	// it will be removed from the list
}

void SVT3_PacketEvent(netadr_t from, QMsg* msg)
{
	// check for connectionless packet (0xffffffff) first
	if (msg->cursize >= 4 && *(int*)msg->_data == -1)
	{
		SVT3_ConnectionlessPacket(from, msg);
		return;
	}

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	msg->BeginReadingOOB();
	msg->ReadLong();				// sequence number
	int qport = msg->ReadShort() & 0xffff;

	// find which client the message is from
	client_t* cl = svs.clients;
	for (int i = 0; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == CS_FREE)
		{
			continue;
		}
		if (!SOCK_CompareBaseAdr(from, cl->netchan.remoteAddress))
		{
			continue;
		}
		// it is possible to have multiple clients from a single IP
		// address, so they are differentiated by the qport variable
		if (cl->netchan.qport != qport)
		{
			continue;
		}

		// the IP port can't be used to differentiate them, because
		// some address translating routers periodically change UDP
		// port assignments
		if (cl->netchan.remoteAddress.port != from.port)
		{
			common->Printf("SVT3_PacketEvent: fixing up a translated port\n");
			cl->netchan.remoteAddress.port = from.port;
		}

		// make sure it is a valid, in sequence packet
		if (SVT3_Netchan_Process(cl, msg))
		{
			// zombie clients still need to do the Netchan_Process
			// to make sure they don't need to retransmit the final
			// reliable message, but they don't do any other processing
			if (cl->state != CS_ZOMBIE)
			{
				cl->q3_lastPacketTime = svs.q3_time;	// don't timeout
				SVT3_ExecuteClientMessage(cl, msg);
			}
		}
		return;
	}

	// if we received a sequenced packet from an address we don't recognize,
	// send an out of band disconnect packet to it
	NET_OutOfBandPrint(NS_SERVER, from, "disconnect");
}

//	Player movement occurs as a result of packet events, which
// happen before SVT3_Frame is called
void SVT3_Frame(int msec)
{
	// the menu kills the server with this cvar
	if (svt3_killserver->integer)
	{
		SVT3_Shutdown("Server was killed.\n");
		Cvar_Set("sv_killserver", "0");
		return;
	}

	if (!com_sv_running->integer)
	{
		return;
	}

	// allow pause if only the local client is connected
	if (SVT3_CheckPaused())
	{
		return;
	}

	int frameStartTime = 0;
	if (com_dedicated->integer)
	{
		frameStartTime = Sys_Milliseconds();
	}

	// if it isn't time for the next frame, do nothing
	if (svt3_fps->integer < 1)
	{
		Cvar_Set("sv_fps", "10");
	}
	int frameMsec = 1000 / svt3_fps->integer;

	sv.q3_timeResidual += msec;

	if (!com_dedicated->integer)
	{
		SVT3_BotFrame(svs.q3_time + sv.q3_timeResidual);
	}

	if (com_dedicated->integer && sv.q3_timeResidual < frameMsec)
	{
		// NET_Sleep will give the OS time slices until either get a packet
		// or time enough for a server frame has gone by
		NET_Sleep(frameMsec - sv.q3_timeResidual);
		return;
	}

	// if time is about to hit the 32nd bit, kick all clients
	// and clear sv.time, rather
	// than checking for negative time wraparound everywhere.
	// 2giga-milliseconds = 23 days, so it won't be too often
	if (svs.q3_time > 0x70000000)
	{
		char mapname[MAX_QPATH];
		String::NCpyZ(mapname, svt3_mapname->string, MAX_QPATH);
		SVT3_Shutdown("Restarting server due to time wrapping");
		// TTimo
		// show_bug.cgi?id=388
		// there won't be a map_restart if you have shut down the server
		// since it doesn't restart a non-running server
		// instead, re-run the current map
		Cbuf_AddText(va("map %s\n", mapname));
		return;
	}
	// this can happen considerably earlier when lots of clients play and the map doesn't change
	if (svs.q3_nextSnapshotEntities >= 0x7FFFFFFE - svs.q3_numSnapshotEntities)
	{
		char mapname[MAX_QPATH];
		String::NCpyZ(mapname, svt3_mapname->string, MAX_QPATH);
		SVT3_Shutdown("Restarting server due to numSnapshotEntities wrapping");
		// TTimo see above
		Cbuf_AddText(va("map %s\n", mapname));
		return;
	}

	if (sv.q3_restartTime && svs.q3_time >= sv.q3_restartTime)
	{
		sv.q3_restartTime = 0;
		Cbuf_AddText("map_restart 0\n");
		return;
	}

	// update infostrings if anything has been changed
	if (cvar_modifiedFlags & CVAR_SERVERINFO)
	{
		SVT3_SetConfigstring(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO;
	}
	if (cvar_modifiedFlags & CVAR_SERVERINFO_NOUPDATE)
	{
		SVT3_SetConfigstringNoUpdate(Q3CS_SERVERINFO, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_SERVERINFO_NOUPDATE;
	}
	if (cvar_modifiedFlags & CVAR_SYSTEMINFO)
	{
		SVT3_SetConfigstring(Q3CS_SYSTEMINFO, Cvar_InfoString(CVAR_SYSTEMINFO, BIG_INFO_STRING));
		cvar_modifiedFlags &= ~CVAR_SYSTEMINFO;
	}
	// NERVE - SMF
	if (GGameType & (GAME_WolfMP | GAME_ET) && cvar_modifiedFlags & CVAR_WOLFINFO)
	{
		SVT3_SetConfigstring(GGameType & GAME_ET ? ETCS_WOLFINFO : WMCS_WOLFINFO, Cvar_InfoString(CVAR_WOLFINFO, MAX_INFO_STRING_Q3));
		cvar_modifiedFlags &= ~CVAR_WOLFINFO;
	}

	int startTime;
	if (com_speeds->integer)
	{
		startTime = Sys_Milliseconds();
	}
	else
	{
		startTime = 0;	// quite a compiler warning
	}

	// update ping based on the all received frames
	SVT3_CalcPings();

	if (com_dedicated->integer)
	{
		SVT3_BotFrame(svs.q3_time);
	}

	// run the game simulation in chunks
	while (sv.q3_timeResidual >= frameMsec)
	{
		sv.q3_timeResidual -= frameMsec;
		svs.q3_time += frameMsec;

		// let everything in the world think and move
		SVT3_GameRunFrame(svs.q3_time);
	}

	if (com_speeds->integer)
	{
		t3time_game = Sys_Milliseconds() - startTime;
	}

	// check timeouts
	SVT3_CheckTimeouts();

	// send messages back to the clients
	SVT3_SendClientMessages();

	// send a heartbeat to the master if needed
	SVT3_MasterHeartbeat(GGameType & GAME_Quake3 ? Q3HEARTBEAT_GAME :
		GGameType & GAME_ET ? ETHEARTBEAT_GAME : WSMHEARTBEAT_GAME);

	if (GGameType & GAME_ET && com_dedicated->integer)
	{
		int frameEndTime = Sys_Milliseconds();

		svs.et_totalFrameTime += (frameEndTime - frameStartTime);
		svs.et_currentFrameIndex++;

		if (svs.et_currentFrameIndex == ETSERVER_PERFORMANCECOUNTER_FRAMES)
		{
			int averageFrameTime;

			averageFrameTime = svs.et_totalFrameTime / ETSERVER_PERFORMANCECOUNTER_FRAMES;

			svs.et_sampleTimes[svs.et_currentSampleIndex % SERVER_PERFORMANCECOUNTER_SAMPLES] = averageFrameTime;
			svs.et_currentSampleIndex++;

			if (svs.et_currentSampleIndex > SERVER_PERFORMANCECOUNTER_SAMPLES)
			{
				int totalTime = 0;
				for (int i = 0; i < SERVER_PERFORMANCECOUNTER_SAMPLES; i++)
				{
					totalTime += svs.et_sampleTimes[i];
				}

				if (!totalTime)
				{
					totalTime = 1;
				}

				averageFrameTime = totalTime / SERVER_PERFORMANCECOUNTER_SAMPLES;

				svs.et_serverLoad = (averageFrameTime / (float)frameMsec) * 100;
			}

			svs.et_totalFrameTime = 0;
			svs.et_currentFrameIndex = 0;
		}
	}
	else
	{
		svs.et_serverLoad = -1;
	}
}
