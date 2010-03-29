
#include "qwsvdef.h"

qboolean	sv_allow_cheats;

int fp_messages=4, fp_persecond=4, fp_secondsdead=10;
char fp_msg[255] = { 0 };
extern cvar_t cl_warncmd;

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
void SV_SetMaster_f (void)
{
	char	data[2];
	int		i;

	memset (&master_adr, 0, sizeof(master_adr));

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		if (!strcmp(Cmd_Argv(i), "none") || !NET_StringToAdr (Cmd_Argv(i), &master_adr[i-1]))
		{
			Con_Printf ("Setting nomaster mode.\n");
			return;
		}
		if (master_adr[i-1].port == 0)
			master_adr[i-1].port = BigShort (PORT_MASTER);

		Con_Printf ("Master server at %s\n", NET_AdrToString (master_adr[i-1]));

		Con_Printf ("Sending a ping.\n");

		data[0] = A2A_PING;
		data[1] = 0;
		NET_SendPacket (2, data, master_adr[i-1]);
	}

	svs.last_heartbeat = -99999;
}


/*
==================
SV_Quit_f
==================
*/
void SV_Quit_f (void)
{
	SV_FinalMessage ("server shutdown\n");
	Con_Printf ("Shutting down.\n");
	SV_Shutdown ();
	Sys_Quit ();
}

/*
============
SV_Logfile_f
============
*/
void SV_Logfile_f (void)
{
	char	name[MAX_OSPATH];

	if (sv_logfile)
	{
		Con_Printf ("File logging off.\n");
		fclose (sv_logfile);
		sv_logfile = NULL;
		return;
	}

	sprintf (name, "%s/qconsole.log", com_gamedir);
	Con_Printf ("Logging text to %s.\n", name);
	sv_logfile = fopen (name, "w");
	if (!sv_logfile)
		Con_Printf ("failed.\n");
}


/*
============
SV_Fraglogfile_f
============
*/
void SV_Fraglogfile_f (void)
{
	char	name[MAX_OSPATH];
	int		i;

	if (sv_fraglogfile)
	{
		Con_Printf ("Frag file logging off.\n");
		fclose (sv_fraglogfile);
		sv_fraglogfile = NULL;
		return;
	}

	// find an unused name
	for (i=0 ; i<1000 ; i++)
	{
		sprintf (name, "%s/frag_%i.log", com_gamedir, i);
		sv_fraglogfile = fopen (name, "r");
		if (!sv_fraglogfile)
		{	// can't read it, so create this one
			sv_fraglogfile = fopen (name, "w");
			if (!sv_fraglogfile)
				i=1000;	// give error
			break;
		}
		fclose (sv_fraglogfile);
	}
	if (i==1000)
	{
		Con_Printf ("Can't open any logfiles.\n");
		sv_fraglogfile = NULL;
		return;
	}

	Con_Printf ("Logging frags to %s.\n", name);
}


/*
==================
SV_SetPlayer

Sets host_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;

	idnum = atoi(Cmd_Argv(1));

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == idnum)
		{
			host_client = cl;
			sv_player = host_client->edict;
			return true;
		}
	}
	Con_Printf ("Userid %i is not on the server\n", idnum);
	return false;
}


/*
==================
SV_God_f

Sets client to godmode
==================
*/
void SV_God_f (void)
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	sv_player->v.flags = (int)sv_player->v.flags ^ FL_GODMODE;
	if (!((int)sv_player->v.flags & FL_GODMODE) )
		SV_ClientPrintf (host_client, PRINT_HIGH, "godmode OFF\n");
	else
		SV_ClientPrintf (host_client, PRINT_HIGH, "godmode ON\n");
}


void SV_Noclip_f (void)
{
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	if (sv_player->v.movetype != MOVETYPE_NOCLIP)
	{
		sv_player->v.movetype = MOVETYPE_NOCLIP;
		SV_ClientPrintf (host_client, PRINT_HIGH, "noclip ON\n");
	}
	else
	{
		sv_player->v.movetype = MOVETYPE_WALK;
		SV_ClientPrintf (host_client, PRINT_HIGH, "noclip OFF\n");
	}
}


/*
==================
SV_Give_f
==================
*/
void SV_Give_f (void)
{
	char	*t;
	int		v;
	
	if (!sv_allow_cheats)
	{
		Con_Printf ("You must run the server with -cheats to enable this command.\n");
		return;
	}
	
	if (!SV_SetPlayer ())
		return;

	t = Cmd_Argv(2);
	v = atoi (Cmd_Argv(3));
	
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
		sv_player->v.items = (int)sv_player->v.items | IT_SHOTGUN<< (t[0] - '2');
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
		sv_player->v.health = v;
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
void SV_Map_f (void)
{
	char	level[MAX_QPATH];
	char	expanded[MAX_QPATH];
	FILE	*f;
	char	_startspot[MAX_QPATH];
	char	*startspot;

	if (Cmd_Argc() < 2)
	{
		Con_Printf ("map <levelname> : continue game on a new level\n");
		return;
	}
	strcpy (level, Cmd_Argv(1));
	if (Cmd_Argc() == 2)
	{
		startspot = NULL;
	}
	else
	{
		strcpy (_startspot, Cmd_Argv(2));
		startspot = _startspot;
	}

#if 0
	if (!strcmp (level, "e1m8"))
	{	// QuakeWorld can't go to e1m8
		SV_BroadcastPrintf (PRINT_HIGH, "can't go to low grav level in HexenWorld...\n");
		strcpy (level, "e1m5");
	}
#endif

	// check to make sure the level exists
	sprintf (expanded, "maps/%s.bsp", level);
	COM_FOpenFile (expanded, &f, false);
	if (!f)
	{
		Con_Printf ("Can't find %s\n", expanded);
		return;
	}
	fclose (f);

	SV_BroadcastCommand ("changing\n");
	SV_SendMessagesToAll ();

	SV_SpawnServer (level, startspot);

	SV_BroadcastCommand ("reconnect\n");
}




/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void)
{
	int			i;
	client_t	*cl;
	int			uid;

	uid = atoi(Cmd_Argv(1));
	
	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (!cl->state)
			continue;
		if (cl->userid == uid)
		{
			SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", cl->name);
			// print directly, because the dropped client won't get the
			// SV_BroadcastPrintf message
			SV_ClientPrintf (cl, PRINT_HIGH, "You were kicked from the game\n");
			SV_DropClient (cl); 
			
			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG(sv_player);
			PR_ExecuteProgram (pr_global_struct->ClientKill);
			return;
		}
	}

	Con_Printf ("Couldn't find user number %i\n", uid);
}


/*
==================
SV_Smite_f
==================
*/
void SV_Smite_f (void)
{
	int			i;
	client_t	*cl;
	int			uid;
	int		old_self;

	uid = atoi(Cmd_Argv(1));
	
	for (i = 0, cl = svs.clients; i < MAX_CLIENTS; i++, cl++)
	{
		if (cl->state!=cs_spawned)
			continue;
		if (cl->userid == uid)
		{
			if (cl->old_v.health <= 0)
			{
				Con_Printf("%s is already dead!\n", cl->name);
				return;
			}
			SV_BroadcastPrintf (PRINT_HIGH, "%s was Smitten by GOD!\n", cl->name);

//save this state
			old_self = pr_global_struct->self;
			
//call the hc SmitePlayer function
			pr_global_struct->time = sv.time;
			pr_global_struct->self = EDICT_TO_PROG(cl->edict);
			PR_ExecuteProgram (pr_global_struct->SmitePlayer);

//restore current state
			pr_global_struct->self = old_self;
			return;
		}
	}
	Con_Printf ("Couldn't find user number %i\n", uid);
}

/*
================
SV_Status_f
================
*/
void SV_Status_f (void)
{
	int			i, j, l, num_min, num_sec;
	client_t	*cl;
	float		cpu, avg, pak, t_limit,f_limit;
	char		*s;
	extern		redirect_t	sv_redirected;


	cpu = (svs.stats.latched_active+svs.stats.latched_idle);
	if (cpu)
		cpu = 100*svs.stats.latched_active/cpu;
	avg = 1000*svs.stats.latched_active / STATFRAMES;
	pak = (float)svs.stats.latched_packets/ STATFRAMES;

	Con_Printf ("net address      : %s\n",NET_AdrToString (net_local_adr));
	Con_Printf ("cpu utilization  : %3i%%\n",(int)cpu);
	Con_Printf ("avg response time: %i ms\n",(int)avg);
	Con_Printf ("packets/frame    : %5.2f\n", pak);
	t_limit = Cvar_VariableValue("timelimit");
	f_limit = Cvar_VariableValue("fraglimit");
	if(dmMode.value==DM_SIEGE)
	{
		num_min = floor((t_limit*60)-sv.time);
		num_sec = (int)(t_limit - num_min)%60;
		num_sec = floor(num_sec);
		num_min = floor((num_min - num_sec)/60);
		Con_Printf ("timeleft         : %i:", num_min);
		Con_Printf ("%2i\n", num_sec);
		Con_Printf ("deflosses        : %3i/%3i\n", floor(pr_global_struct->defLosses),floor(f_limit));
		Con_Printf ("attlosses        : %3i/%3i\n", floor(pr_global_struct->attLosses),floor(f_limit*2));
	}
	else
	{
		Con_Printf ("time             : %5.2f\n", sv.time);
		Con_Printf ("timelimit        : %i\n", t_limit);
		Con_Printf ("fraglimit        : %i\n", f_limit);
	}
	
// min fps lat drp
	if (sv_redirected != RD_NONE) 
	{
		// most remote clients are 40 columns
		//           0123456789012345678901234567890123456789
		Con_Printf ("name               userid frags\n");
        Con_Printf ("  address          rate ping drop\n");
		Con_Printf ("  ---------------- ---- ---- -----\n");
		for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (!cl->state)
				continue;

			Con_Printf ("%-16.16s  ", cl->name);

			Con_Printf ("%6i %5i", cl->userid, (int)cl->edict->v.frags);
			if (cl->spectator)
				Con_Printf(" (s)\n");
			else			
				Con_Printf("\n");

			s = NET_BaseAdrToString ( cl->netchan.remote_address);
			Con_Printf ("  %-16.16s", s);
			if (cl->state == cs_connected)
			{
				Con_Printf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie)
			{
				Con_Printf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %5.2f\n"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence);
		}
	} 
	else 
	{
		Con_Printf ("frags userid address         name            rate ping drop  siege\n");
		Con_Printf ("----- ------ --------------- --------------- ---- ---- ----- -----\n");
		for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (!cl->state)
				continue;
			Con_Printf ("%5i %6i ", (int)cl->edict->v.frags,  cl->userid);

			s = NET_BaseAdrToString ( cl->netchan.remote_address);
			Con_Printf ("%s", s);
			l = 16 - strlen(s);
			for (j=0 ; j<l ; j++)
				Con_Printf (" ");
			
			Con_Printf ("%s", cl->name);
			l = 16 - strlen(cl->name);
			for (j=0 ; j<l ; j++)
				Con_Printf (" ");
			if (cl->state == cs_connected)
			{
				Con_Printf ("CONNECTING\n");
				continue;
			}
			if (cl->state == cs_zombie)
			{
				Con_Printf ("ZOMBIE\n");
				continue;
			}
			Con_Printf ("%4i %4i %5.2f"
				, (int)(1000*cl->netchan.frame_rate)
				, (int)SV_CalcPing (cl)
				, 100.0*cl->netchan.drop_count / cl->netchan.incoming_sequence);

			if (cl->spectator)
				Con_Printf(" (s)\n");
			else			
			{
				Con_Printf(" ");
				switch(cl->playerclass)
				{
				case CLASS_PALADIN:
					Con_Printf("P");
					break;
				case CLASS_CLERIC:
					Con_Printf("C");
					break;
				case CLASS_NECROMANCER:
					Con_Printf("N");
					break;
				case CLASS_THEIF:
					Con_Printf("A");
					break;
				case CLASS_DEMON:
					Con_Printf("S");
					break;
				case CLASS_DWARF:
					Con_Printf("D");
					break;
				default:
					Con_Printf("?");
					break;
				}
				switch(cl->siege_team)
				{
				case ST_DEFENDER:
					Con_Printf("D");
					break;
				case ST_ATTACKER:
					Con_Printf("A");
					break;
				default:
					Con_Printf("?");
					break;
				}
				if((int)cl->old_v.flags2&65536)//defender of crown
					Con_Printf("D");
				else 
					Con_Printf("-");
				if((int)cl->old_v.flags2&524288)//has siege key
					Con_Printf("K");
				else 
					Con_Printf("-");
				Con_Printf("\n");
			}

				
		}
	}
	Con_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	if(dmMode.value==DM_SIEGE)
		strcpy (text, "GOD SAYS: ");
	else
		strcpy (text, "ServerAdmin: ");

	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < MAX_CLIENTS; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -9999;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
char *CopyString(char *s);
void SV_Serverinfo_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Server info settings:\n");
		Info_Print (svs.info);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: serverinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey (svs.info, Cmd_Argv(1), Cmd_Argv(2), MAX_SERVERINFO_STRING);

	// if this is a cvar, change it too	
	var = Cvar_FindVar (Cmd_Argv(1));
	if (var)
	{
		Z_Free (var->string);	// free the old value string	
		var->string = CopyString (Cmd_Argv(2));
		var->value = atof (var->string);
	}

	SV_BroadcastCommand ("fullserverinfo \"%s\"\n", svs.info);
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
char *CopyString(char *s);
void SV_Localinfo_f (void)
{
	cvar_t	*var;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Local info settings:\n");
		Info_Print (localinfo);
		return;
	}

	if (Cmd_Argc() != 3)
	{
		Con_Printf ("usage: localinfo [ <key> <value> ]\n");
		return;
	}

	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
	}
	Info_SetValueForKey (localinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_LOCALINFO_STRING);
}


/*
===========
SV_User_f

Examine a users info strings
===========
*/
void SV_User_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Info_Print (host_client->userinfo);
}

/*
================
SV_Gamedir

Sets the fake *gamedir to a different directory.
================
*/
void SV_Gamedir (void)
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current *gamedir: %s\n", Info_ValueForKey (svs.info, "*gamedir"));
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: sv_gamedir <newgamedir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("*Gamedir should be a single filename, not a path\n");
		return;
	}

	Info_SetValueForStarKey (svs.info, "*gamedir", dir, MAX_SERVERINFO_STRING);
}

/*
================
SV_Floodport_f

Sets the gamedir and path to a different directory.
================
*/

void SV_Floodprot_f (void)
{
	int arg1, arg2, arg3;
	
	if (Cmd_Argc() == 1)
	{
		if (fp_messages) {
			Con_Printf ("Current floodprot settings: \nAfter %d msgs per %d seconds, silence for %d seconds\n", 
				fp_messages, fp_persecond, fp_secondsdead);
			return;
		} else
			Con_Printf ("No floodprots enabled.\n");
	}

	if (Cmd_Argc() != 4)
	{
		Con_Printf ("Usage: floodprot <# of messages> <per # of seconds> <seconds to silence>\n");
		Con_Printf ("Use floodprotmsg to set a custom message to say to the flooder.\n");
		return;
	}

	arg1 = atoi(Cmd_Argv(1));
	arg2 = atoi(Cmd_Argv(2));
	arg3 = atoi(Cmd_Argv(3));

	if (arg1<=0 || arg2 <= 0 || arg3<=0) {
		Con_Printf ("All values must be positive numbers\n");
		return;
	}
	
	if (arg1 > 10) {
		Con_Printf ("Can only track up to 10 messages.\n");
		return;
	}

	fp_messages	= arg1;
	fp_persecond = arg2;
	fp_secondsdead = arg3;
}

void SV_Floodprotmsg_f (void)
{
	if (Cmd_Argc() == 1) {
		Con_Printf("Current msg: %s\n", fp_msg);
		return;
	} else if (Cmd_Argc() != 2) {
		Con_Printf("Usage: floodprotmsg \"<message>\"\n");
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
char	gamedirfile[MAX_OSPATH];
void SV_Gamedir_f (void)
{
	char			*dir;

	if (Cmd_Argc() == 1)
	{
		Con_Printf ("Current gamedir: %s\n", com_gamedir);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("Usage: gamedir <newdir>\n");
		return;
	}

	dir = Cmd_Argv(1);

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	COM_Gamedir (dir);
	Info_SetValueForStarKey (svs.info, "*gamedir", dir, MAX_SERVERINFO_STRING);
}


/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	if (COM_CheckParm ("-cheats"))
	{
		sv_allow_cheats = true;
		Info_SetValueForStarKey (svs.info, "*cheats", "ON", MAX_SERVERINFO_STRING);
	}

	Cmd_AddCommand ("logfile", SV_Logfile_f);
	Cmd_AddCommand ("fraglogfile", SV_Fraglogfile_f);

	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("smite", SV_Smite_f);

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	Cmd_AddCommand ("say", SV_ConSay_f);
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("quit", SV_Quit_f);
	Cmd_AddCommand ("god", SV_God_f);
	Cmd_AddCommand ("give", SV_Give_f);
	Cmd_AddCommand ("noclip", SV_Noclip_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("localinfo", SV_Localinfo_f);
	Cmd_AddCommand ("user", SV_User_f);
	Cmd_AddCommand ("gamedir", SV_Gamedir_f);
	Cmd_AddCommand ("sv_gamedir", SV_Gamedir);
	Cmd_AddCommand ("floodprot", SV_Floodprot_f);
	Cmd_AddCommand ("floodprotmsg", SV_Floodprotmsg_f);

	cl_warncmd.value = 1;
}
