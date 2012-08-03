
#include "qwsvdef.h"

qboolean sv_allow_cheats;

int fp_messages = 4, fp_persecond = 4, fp_secondsdead = 10;
char fp_msg[255] = { 0 };
extern Cvar* cl_warncmd;

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Make a master server current
====================
*/
void SV_SetMaster_f(void)
{
	char data[2];
	int i;

	Com_Memset(&master_adr, 0, sizeof(master_adr));

	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (!String::Cmp(Cmd_Argv(i), "none") || !SOCK_StringToAdr(Cmd_Argv(i), &master_adr[i - 1], PORT_MASTER))
		{
			common->Printf("Setting nomaster mode.\n");
			return;
		}

		common->Printf("Master server at %s\n", SOCK_AdrToString(master_adr[i - 1]));

		common->Printf("Sending a ping.\n");

		data[0] = A2A_PING;
		data[1] = 0;
		NET_SendPacket(NS_SERVER, 2, data, master_adr[i - 1]);
	}

	svs.qh_last_heartbeat = -99999;
}


/*
==================
SV_Quit_f
==================
*/
void SV_Quit_f(void)
{
	SV_FinalMessage("server shutdown\n");
	common->Printf("Shutting down.\n");
	SV_Shutdown();
	Sys_Quit();
}

/*
============
SV_Logfile_f
============
*/
void SV_Logfile_f(void)
{
	char name[MAX_OSPATH];

	if (sv_logfile)
	{
		common->Printf("File logging off.\n");
		FS_FCloseFile(sv_logfile);
		sv_logfile = 0;
		return;
	}

	sprintf(name, "qconsole.log");
	common->Printf("Logging text to %s.\n", name);
	sv_logfile = FS_FOpenFileWrite(name);
	if (!sv_logfile)
	{
		common->Printf("failed.\n");
	}
}


/*
============
SV_Fraglogfile_f
============
*/
void SV_Fraglogfile_f(void)
{
	char name[MAX_OSPATH];
	int i;

	if (svqhw_fraglogfile)
	{
		common->Printf("Frag file logging off.\n");
		FS_FCloseFile(svqhw_fraglogfile);
		svqhw_fraglogfile = 0;
		return;
	}

	// find an unused name
	for (i = 0; i < 1000; i++)
	{
		sprintf(name, "frag_%i.log", i);
		FS_FOpenFileRead(name, &svqhw_fraglogfile, true);
		if (!svqhw_fraglogfile)
		{
			// can't read it, so create this one
			svqhw_fraglogfile = FS_FOpenFileWrite(name);
			if (!svqhw_fraglogfile)
			{
				i = 1000;	// give error
			}
			break;
		}
		FS_FCloseFile(svqhw_fraglogfile);
	}
	if (i == 1000)
	{
		common->Printf("Can't open any logfiles.\n");
		svqhw_fraglogfile = 0;
		return;
	}

	common->Printf("Logging frags to %s.\n", name);
}


/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer(void)
{
	client_t* cl;
	int i;
	int idnum;

	idnum = String::Atoi(Cmd_Argv(1));

	for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == idnum)
		{
			host_client = cl;
			sv_player = host_client->qh_edict;
			return true;
		}
	}
	common->Printf("Userid %i is not on the server\n", idnum);
	return false;
}


/*
==================
SV_God_f

Sets client to godmode
==================
*/
void SV_God_f(void)
{
	if (!sv_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	sv_player->SetFlags((int)sv_player->GetFlags() ^ QHFL_GODMODE);
	if (!((int)sv_player->GetFlags() & QHFL_GODMODE))
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "godmode OFF\n");
	}
	else
	{
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "godmode ON\n");
	}
}


void SV_Noclip_f(void)
{
	if (!sv_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	if (sv_player->GetMoveType() != QHMOVETYPE_NOCLIP)
	{
		sv_player->SetMoveType(QHMOVETYPE_NOCLIP);
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		sv_player->SetMoveType(QHMOVETYPE_WALK);
		SVQH_ClientPrintf(host_client, PRINT_HIGH, "noclip OFF\n");
	}
}


/*
==================
SV_Give_f
==================
*/
void SV_Give_f(void)
{
	char* t;
	int v;

	if (!sv_allow_cheats)
	{
		common->Printf("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	t = Cmd_Argv(2);
	v = String::Atoi(Cmd_Argv(3));

	switch (t[0])
	{
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		sv_player->SetItems((int)sv_player->GetItems() | IT_SHOTGUN << (t[0] - '2'));
		break;

	case 's':
//rjr		sv_player->v.ammo_shells = v;
		break;
	case 'n':
//rjr		sv_player->v.ammo_nails = v;
		break;
	case 'r':
//rjr		sv_player->v.ammo_rockets = v;
		break;
	case 'h':
		sv_player->SetHealth(v);
		break;
	case 'c':
//rjr		sv_player->v.ammo_cells = v;
		break;
	}
}


/*
======================
SV_Map_f

handle a
map <mapname>
command from the console or progs.
======================
*/
void SV_Map_f(void)
{
	char level[MAX_QPATH];
	char expanded[MAX_QPATH];
	fileHandle_t f;
	char _startspot[MAX_QPATH];
	char* startspot;

	if (Cmd_Argc() < 2)
	{
		common->Printf("map <levelname> : continue game on a new level\n");
		return;
	}
	String::Cpy(level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		String::Cpy(_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

#if 0
	if (!String::Cmp(level, "e1m8"))
	{	// QuakeWorld can't go to e1m8
		SVQH_BroadcastPrintf(PRINT_HIGH, "can't go to low grav level in HexenWorld...\n");
		String::Cpy(level, "e1m5");
	}
#endif

	// check to make sure the level exists
	sprintf(expanded, "maps/%s.bsp", level);
	FS_FOpenFileRead(expanded, &f, true);
	if (!f)
	{
		common->Printf("Can't find %s\n", expanded);
		return;
	}
	FS_FCloseFile(f);

	SVQH_BroadcastCommand("changing\n");
	SV_SendMessagesToAll();

	SV_SpawnServer(level, startspot);

	SVQH_BroadcastCommand("reconnect\n");
}




/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f(void)
{
	int i;
	client_t* cl;
	int uid;

	uid = String::Atoi(Cmd_Argv(1));

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (!cl->state)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s was kicked\n", cl->name);
			// print directly, because the dropped client won't get the
			// SVQH_BroadcastPrintf message
			SVQH_ClientPrintf(cl, PRINT_HIGH, "You were kicked from the game\n");
			SVQHW_DropClient(cl);

			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(sv_player);
			PR_ExecuteProgram(*pr_globalVars.ClientKill);
			return;
		}
	}

	common->Printf("Couldn't find user number %i\n", uid);
}


/*
==================
SV_Smite_f
==================
*/
void SV_Smite_f(void)
{
	int i;
	client_t* cl;
	int uid;
	int old_self;

	uid = String::Atoi(Cmd_Argv(1));

	for (i = 0, cl = svs.clients; i < MAX_CLIENTS_QHW; i++, cl++)
	{
		if (cl->state != CS_ACTIVE)
		{
			continue;
		}
		if (cl->qh_userid == uid)
		{
			if (cl->h2_old_v.health <= 0)
			{
				common->Printf("%s is already dead!\n", cl->name);
				return;
			}
			SVQH_BroadcastPrintf(PRINT_HIGH, "%s was Smitten by GOD!\n", cl->name);

//save this state
			old_self = *pr_globalVars.self;

//call the hc SmitePlayer function
			*pr_globalVars.time = sv.qh_time;
			*pr_globalVars.self = EDICT_TO_PROG(cl->qh_edict);
			PR_ExecuteProgram(*pr_globalVars.SmitePlayer);

//restore current state
			*pr_globalVars.self = old_self;
			return;
		}
	}
	common->Printf("Couldn't find user number %i\n", uid);
}

/*
================
SV_Status_f
================
*/
void SV_Status_f(void)
{
	int i, j, l, num_min, num_sec;
	client_t* cl;
	float cpu, avg, pak, t_limit,f_limit;
	const char* s;
	extern redirect_t sv_redirected;


	cpu = (svs.qh_stats.latched_active + svs.qh_stats.latched_idle);
	if (cpu)
	{
		cpu = 100 * svs.qh_stats.latched_active / cpu;
	}
	avg = 1000 * svs.qh_stats.latched_active / STATFRAMES;
	pak = (float)svs.qh_stats.latched_packets / STATFRAMES;

	SOCK_ShowIP();
	common->Printf("port             : %d\n", sv_net_port);
	common->Printf("cpu utilization  : %3i%%\n",(int)cpu);
	common->Printf("avg response time: %i ms\n",(int)avg);
	common->Printf("packets/frame    : %5.2f\n", pak);
	t_limit = Cvar_VariableValue("timelimit");
	f_limit = Cvar_VariableValue("fraglimit");
	if (hw_dmMode->value == HWDM_SIEGE)
	{
		num_min = floor((t_limit * 60) - sv.qh_time);
		num_sec = (int)(t_limit - num_min) % 60;
		num_min = (num_min - num_sec) / 60;
		common->Printf("timeleft         : %i:", num_min);
		common->Printf("%2i\n", num_sec);
		common->Printf("deflosses        : %3i/%3i\n", floor(*pr_globalVars.defLosses),floor(f_limit));
		common->Printf("attlosses        : %3i/%3i\n", floor(*pr_globalVars.attLosses),floor(f_limit * 2));
	}
	else
	{
		common->Printf("time             : %5.2f\n", sv.qh_time);
		common->Printf("timelimit        : %i\n", t_limit);
		common->Printf("fraglimit        : %i\n", f_limit);
	}

// min fps lat drp
	if (sv_redirected != RD_NONE)
	{
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		common->Printf("name               userid frags\n");
		common->Printf("  address          ping drop\n");
		common->Printf("  ---------------- ---- -----\n");
		for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (!cl->state)
			{
				continue;
			}

			common->Printf("%-16.16s  ", cl->name);

			common->Printf("%6i %5i", cl->qh_userid, (int)cl->qh_edict->GetFrags());
			if (cl->qh_spectator)
			{
				common->Printf(" (s)\n");
			}
			else
			{
				common->Printf("\n");
			}

			s = SOCK_BaseAdrToString(cl->netchan.remoteAddress);
			common->Printf("  %-16.16s", s);
			if (cl->state == CS_CONNECTED)
			{
				common->Printf("CONNECTING\n");
				continue;
			}
			if (cl->state == CS_ZOMBIE)
			{
				common->Printf("ZOMBIE\n");
				continue;
			}
			common->Printf("%4i %5.2f\n",
				(int)SVQH_CalcPing(cl),
				100.0 * cl->netchan.dropCount / cl->netchan.incomingSequence);
		}
	}
	else
	{
		common->Printf("frags userid address         name            ping drop  siege\n");
		common->Printf("----- ------ --------------- --------------- ---- ----- -----\n");
		for (i = 0,cl = svs.clients; i < MAX_CLIENTS_QHW; i++,cl++)
		{
			if (!cl->state)
			{
				continue;
			}
			common->Printf("%5i %6i ", (int)cl->qh_edict->GetFrags(),  cl->qh_userid);

			s = SOCK_BaseAdrToString(cl->netchan.remoteAddress);
			common->Printf("%s", s);
			l = 16 - String::Length(s);
			for (j = 0; j < l; j++)
				common->Printf(" ");

			common->Printf("%s", cl->name);
			l = 16 - String::Length(cl->name);
			for (j = 0; j < l; j++)
				common->Printf(" ");
			if (cl->state == CS_CONNECTED)
			{
				common->Printf("CONNECTING\n");
				continue;
			}
			if (cl->state == CS_ZOMBIE)
			{
				common->Printf("ZOMBIE\n");
				continue;
			}
			common->Printf("%4i %5.2f",
				(int)SVQH_CalcPing(cl),
				100.0 * cl->netchan.dropCount / cl->netchan.incomingSequence);

			if (cl->qh_spectator)
			{
				common->Printf(" (s)\n");
			}
			else
			{
				common->Printf(" ");
				switch (cl->h2_playerclass)
				{
				case CLASS_PALADIN:
					common->Printf("P");
					break;
				case CLASS_CLERIC:
					common->Printf("C");
					break;
				case CLASS_NECROMANCER:
					common->Printf("N");
					break;
				case CLASS_THEIF:
					common->Printf("A");
					break;
				case CLASS_DEMON:
					common->Printf("S");
					break;
				case CLASS_DWARF:
					common->Printf("D");
					break;
				default:
					common->Printf("?");
					break;
				}
				switch (cl->hw_siege_team)
				{
				case HWST_DEFENDER:
					common->Printf("D");
					break;
				case HWST_ATTACKER:
					common->Printf("A");
					break;
				default:
					common->Printf("?");
					break;
				}
				if ((int)cl->h2_old_v.flags2 & 65536)	//defender of crown
				{
					common->Printf("D");
				}
				else
				{
					common->Printf("-");
				}
				if ((int)cl->h2_old_v.flags2 & 524288)	//has siege key
				{
					common->Printf("K");
				}
				else
				{
					common->Printf("-");
				}
				common->Printf("\n");
			}


		}
	}
	common->Printf("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t* client;
	int j;
	char* p;
	char text[1024];

	if (Cmd_Argc() < 2)
	{
		return;
	}

	if (hw_dmMode->value == HWDM_SIEGE)
	{
		String::Cpy(text, "GOD SAYS: ");
	}
	else
	{
		String::Cpy(text, "ServerAdmin: ");
	}

	p = Cmd_ArgsUnmodified();

	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	String::Cat(text, sizeof(text), p);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS_QHW; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f(void)
{
	svs.qh_last_heartbeat = -9999;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Serverinfo_f(void)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Server info settings:\n");
		Info_Print(svs.qh_info);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey(svs.qh_info, Cmd_Argv(1), Cmd_Argv(2), MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value, false);

	// if this is a cvar, change it too
	Cvar_UpdateIfExists(Cmd_Argv(1), Cmd_Argv(2));

	SVQH_BroadcastCommand("fullserverinfo \"%s\"\n", svs.qh_info);
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
void SV_Localinfo_f(void)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Local info settings:\n");
		Info_Print(qhw_localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		common->Printf("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		common->Printf("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey(qhw_localinfo, Cmd_Argv(1), Cmd_Argv(2), QHMAX_LOCALINFO_STRING, 64, 64, !svqh_highchars->value, false);
}


/*
===========
SV_User_f

Examine a users info strings
===========
*/
void SV_User_f(void)
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer())
	{
		return;
	}

	Info_Print(host_client->userinfo);
}

/*
================
SV_Gamedir

Sets the fake *gamedir to a different directory.
================
*/
void SV_Gamedir(void)
{
	char* dir;

	if (Cmd_Argc() == 1)
	{
		common->Printf("Current *gamedir: %s\n", Info_ValueForKey(svs.qh_info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("*Gamedir should be a single filename, not a path\n");
		return;
	}

	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}

/*
================
SV_Floodport_f

Sets the gamedir and path to a different directory.
================
*/

void SV_Floodprot_f(void)
{
	int arg1, arg2, arg3;

	if (Cmd_Argc() == 1)
	{
		if (fp_messages)
		{
			common->Printf("Current floodprot settings: \nAfter %d msgs per %d seconds, silence for %d seconds\n",
				fp_messages, fp_persecond, fp_secondsdead);
			return;
		}
		else
		{
			common->Printf("No floodprots enabled.\n");
		}
	}

	if (Cmd_Argc() != 4)
	{
		common->Printf("Usage: floodprot <# of messages> <per # of seconds> <seconds to silence>\n");
		common->Printf("Use floodprotmsg to set a custom message to say to the flooder.\n");
		return;
	}

	arg1 = String::Atoi(Cmd_Argv(1));
	arg2 = String::Atoi(Cmd_Argv(2));
	arg3 = String::Atoi(Cmd_Argv(3));

	if (arg1 <= 0 || arg2 <= 0 || arg3 <= 0)
	{
		common->Printf("All values must be positive numbers\n");
		return;
	}

	if (arg1 > 10)
	{
		common->Printf("Can only track up to 10 messages.\n");
		return;
	}

	fp_messages = arg1;
	fp_persecond = arg2;
	fp_secondsdead = arg3;
}

void SV_Floodprotmsg_f(void)
{
	if (Cmd_Argc() == 1)
	{
		common->Printf("Current msg: %s\n", fp_msg);
		return;
	}
	else if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: floodprotmsg \"<message>\"\n");
		return;
	}
	sprintf(fp_msg, "%s", Cmd_Argv(1));
}

/*
================
SV_Gamedir_f

Sets the gamedir and path to a different directory.
================
*/
extern char gamedirfile[MAX_OSPATH];
void SV_Gamedir_f(void)
{
	char* dir;

	if (Cmd_Argc() == 1)
	{
		common->Printf("Current gamedir: %s\n", fs_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		common->Printf("Usage: gamedir <newdir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	COM_Gamedir(dir);
	Info_SetValueForKey(svs.qh_info, "*gamedir", dir, MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
}


/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands(void)
{
	if (COM_CheckParm("-cheats"))
	{
		sv_allow_cheats = true;
		Info_SetValueForKey(svs.qh_info, "*cheats", "ON", MAX_SERVERINFO_STRING, 64, 64, !svqh_highchars->value);
	}

	Cmd_AddCommand("logfile", SV_Logfile_f);
	Cmd_AddCommand("fraglogfile", SV_Fraglogfile_f);

	Cmd_AddCommand("kick", SV_Kick_f);
	Cmd_AddCommand("status", SV_Status_f);
	Cmd_AddCommand("smite", SV_Smite_f);

	Cmd_AddCommand("map", SV_Map_f);
	Cmd_AddCommand("setmaster", SV_SetMaster_f);

	Cmd_AddCommand("say", SV_ConSay_f);
	Cmd_AddCommand("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand("quit", SV_Quit_f);
	Cmd_AddCommand("god", SV_God_f);
	Cmd_AddCommand("give", SV_Give_f);
	Cmd_AddCommand("noclip", SV_Noclip_f);
	Cmd_AddCommand("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand("localinfo", SV_Localinfo_f);
	Cmd_AddCommand("user", SV_User_f);
	Cmd_AddCommand("gamedir", SV_Gamedir_f);
	Cmd_AddCommand("sv_gamedir", SV_Gamedir);
	Cmd_AddCommand("floodprot", SV_Floodprot_f);
	Cmd_AddCommand("floodprotmsg", SV_Floodprotmsg_f);

	cl_warncmd->value = 1;
}
