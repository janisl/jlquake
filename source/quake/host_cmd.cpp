/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

/*
==================
Host_Quit_f
==================
*/

extern void M_Menu_Quit_f(void);

void Host_Quit_f(void)
{
#ifndef DEDICATED
	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && cls.state != CA_DEDICATED)
	{
		M_Menu_Quit_f();
		return;
	}
	CL_Disconnect();
#endif
	SVQH_Shutdown(false);

	Sys_Quit();
}


/*
==================
Host_Status_f
==================
*/
void Host_Status_f(void)
{
	client_t* client;
	int seconds;
	int minutes;
	int hours = 0;
	int j;

	if (sv.state == SS_DEAD)
	{
		Cmd_ForwardToServer();
		return;
	}

	common->Printf("host:    %s\n", Cvar_VariableString("hostname"));
	common->Printf("version: " JLQUAKE_VERSION_STRING "\n");
	SOCK_ShowIP();
	common->Printf("map:     %s\n", sv.name);
	common->Printf("players: %i active (%i max)\n\n", net_activeconnections, svs.qh_maxclients);
	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		seconds = (int)(net_time - client->qh_netconnection->connecttime);
		minutes = seconds / 60;
		if (minutes)
		{
			seconds -= (minutes * 60);
			hours = minutes / 60;
			if (hours)
			{
				minutes -= (hours * 60);
			}
		}
		else
		{
			hours = 0;
		}
		common->Printf("#%-2u %-16.16s  %3i  %2i:%02i:%02i\n", j + 1, client->name, (int)client->qh_edict->GetFrags(), hours, minutes, seconds);
		common->Printf("   %s\n", client->qh_netconnection->address);
	}
}

#ifndef DEDICATED

/*
==================
Host_God_f

Sets client to godmode
==================
*/
void Host_God_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Notarget_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Noclip_f(void)
{
	Cmd_ForwardToServer();
}

/*
==================
Host_Fly_f

Sets client to flymode
==================
*/
void Host_Fly_f(void)
{
	Cmd_ForwardToServer();
}

/*
==================
Host_Ping_f

==================
*/
void Host_Ping_f(void)
{
	Cmd_ForwardToServer();
}
#endif

/*
===============================================================================

SERVER TRANSITIONS

===============================================================================
*/

#ifndef DEDICATED
/*
==================
Host_Reconnect_f

This command causes the client to wait for the signon messages again.
This is sent just before a server changes levels
==================
*/
void Host_Reconnect_f(void)
{
	SCRQH_BeginLoadingPlaque();
	clc.qh_signon = 0;		// need new connection messages
}

/*
=====================
Host_Connect_f

User command to connect to server
=====================
*/
void Host_Connect_f(void)
{
	char name[MAX_QPATH];

	cls.qh_demonum = -1;		// stop demo loop in case this fails
	if (clc.demoplaying)
	{
		CL_StopPlayback();
		CL_Disconnect();
	}
	String::Cpy(name, Cmd_Argv(1));
	CL_EstablishConnection(name);
	Host_Reconnect_f();
}

/*
======================
Host_Name_f
======================
*/
void Host_Name_f(void)
{
	char* newName;

	if (Cmd_Argc() == 1)
	{
		common->Printf("\"name\" is \"%s\"\n", clqh_name->string);
		return;
	}
	if (Cmd_Argc() == 2)
	{
		newName = Cmd_Argv(1);
	}
	else
	{
		newName = Cmd_ArgsUnmodified();
	}
	newName[15] = 0;

	if (GGameType & GAME_Hexen2)
	{
		//this is for the fuckers who put braces in the name causing loadgame to crash.
		char* pdest = strchr(newName,'{');
		if (pdest)
		{
			*pdest = 0;	//zap the brace
			common->Printf("Illegal char in name removed!\n");
		}
	}

	if (String::Cmp(clqh_name->string, newName) == 0)
	{
		return;
	}
	Cvar_Set("_cl_name", newName);
	if (cls.state == CA_ACTIVE)
	{
		Cmd_ForwardToServer();
	}
}

#endif

void Host_Version_f(void)
{
	common->Printf("Version %4.2f\n", VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

#ifndef DEDICATED

void Host_Say_f()
{
	Cmd_ForwardToServer();
}

void Host_Say_Team_f(void)
{
	Cmd_ForwardToServer();
}

#endif

void Host_ConSay_f()
{
	client_t* client;
	int j;
	char* p;
	char text[64];

	if (Cmd_Argc() < 2)
	{
		return;
	}

	p = Cmd_ArgsUnmodified();
	// remove quotes if present
	if (*p == '"')
	{
		p++;
		p[String::Length(p) - 1] = 0;
	}

	// turn on color set 1
	sprintf(text, "%c<%s> ", 1, sv_hostname->string);

	j = sizeof(text) - 2 - String::Length(text);	// -2 for /n and null terminator
	if (String::Length(p) > j)
	{
		p[j] = 0;
	}

	String::Cat(text, sizeof(text), p);
	String::Cat(text, sizeof(text), "\n");

	for (j = 0, client = svs.clients; j < svs.qh_maxclients; j++, client++)
	{
		if (client->state != CS_ACTIVE)
		{
			continue;
		}
		SVQH_ClientPrintf(client, 0, "%s", text);
	}
}

#ifndef DEDICATED
void Host_Tell_f(void)
{
	Cmd_ForwardToServer();
}

void Host_Color_f(void)
{
	int top, bottom;
	int playercolor;

	if (Cmd_Argc() == 1)
	{
		common->Printf("\"color\" is \"%i %i\"\n", ((int)clqh_color->value) >> 4, ((int)clqh_color->value) & 0x0f);
		common->Printf("color <0-13> [0-13]\n");
		return;
	}

	if (Cmd_Argc() == 2)
	{
		top = bottom = String::Atoi(Cmd_Argv(1));
	}
	else
	{
		top = String::Atoi(Cmd_Argv(1));
		bottom = String::Atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
	{
		top = 13;
	}
	bottom &= 15;
	if (bottom > 13)
	{
		bottom = 13;
	}

	playercolor = top * 16 + bottom;

	Cvar_SetValue("_cl_color", playercolor);
	if (cls.state == CA_ACTIVE)
	{
		Cmd_ForwardToServer();
	}
}

/*
==================
Host_Kill_f
==================
*/
void Host_Kill_f(void)
{
	Cmd_ForwardToServer();
}


/*
==================
Host_Pause_f
==================
*/
void Host_Pause_f(void)
{
	Cmd_ForwardToServer();
}
#endif

//===========================================================================

/*
==================
Host_Kick_f

Kicks a user off of the server
==================
*/
void Host_Kick_f(void)
{
	const char* who;
	const char* message = NULL;
	int i;
	qboolean byNumber = false;

	if (sv.state == SS_DEAD)
	{
		Cmd_ForwardToServer();
		return;
	}

	if (Cmd_Argc() > 2 && String::Cmp(Cmd_Argv(1), "#") == 0)
	{
		i = String::Atof(Cmd_Argv(2)) - 1;
		if (i < 0 || i >= svs.qh_maxclients)
		{
			return;
		}
		if (svs.clients[i].state < CS_CONNECTED)
		{
			return;
		}
		host_client = &svs.clients[i];
		byNumber = true;
	}
	else
	{
		for (i = 0, host_client = svs.clients; i < svs.qh_maxclients; i++, host_client++)
		{
			if (host_client->state < CS_CONNECTED)
			{
				continue;
			}
			if (String::ICmp(host_client->name, Cmd_Argv(1)) == 0)
			{
				break;
			}
		}
	}

	if (i < svs.qh_maxclients)
	{
#ifndef DEDICATED
		if (cls.state == CA_DEDICATED)
#endif
		{
			who = "Console";
		}
#ifndef DEDICATED
		else
		{
			who = clqh_name->string;
		}
#endif

		if (Cmd_Argc() > 2)
		{
			message = Cmd_ArgsUnmodified();
			String::Parse1(&message);
			if (byNumber)
			{
				message++;							// skip the #
				while (*message == ' ')				// skip white space
					message++;
				message += String::Length(Cmd_Argv(2));	// skip the number
			}
			while (*message && *message == ' ')
				message++;
		}
		if (message)
		{
			SVQH_ClientPrintf(host_client, 0, "Kicked by %s: %s\n", who, message);
		}
		else
		{
			SVQH_ClientPrintf(host_client, 0, "Kicked by %s\n", who);
		}
		SVQH_DropClient(host_client, false);
	}
}

/*
===============================================================================

DEBUGGING TOOLS

===============================================================================
*/

#ifndef DEDICATED
/*
==================
Host_Give_f
==================
*/
void Host_Give_f(void)
{
	Cmd_ForwardToServer();
}

qhedict_t* FindViewthing(void)
{
	int i;
	qhedict_t* e;

	for (i = 0; i < sv.qh_num_edicts; i++)
	{
		e = QH_EDICT_NUM(i);
		if (!String::Cmp(PR_GetString(e->GetClassName()), "viewthing"))
		{
			return e;
		}
	}
	common->Printf("No viewthing on map\n");
	return NULL;
}

/*
==================
Host_Viewmodel_f
==================
*/
void Host_Viewmodel_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}

	m = R_RegisterModel(Cmd_Argv(1));
	if (!m)
	{
		common->Printf("Can't load %s\n", Cmd_Argv(1));
		return;
	}

	e->SetFrame(0);
	cl.model_draw[(int)e->v.modelindex] = m;
}

/*
==================
Host_Viewframe_f
==================
*/
void Host_Viewframe_f(void)
{
	qhedict_t* e;
	int f;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}
	m = cl.model_draw[(int)e->v.modelindex];

	f = String::Atoi(Cmd_Argv(1));
	if (f >= R_ModelNumFrames(m))
	{
		f = R_ModelNumFrames(m) - 1;
	}

	e->SetFrame(f);
}

/*
==================
Host_Viewnext_f
==================
*/
void Host_Viewnext_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}
	m = cl.model_draw[(int)e->v.modelindex];

	e->SetFrame(e->GetFrame() + 1);
	if (e->GetFrame() >= R_ModelNumFrames(m))
	{
		e->SetFrame(R_ModelNumFrames(m) - 1);
	}

	R_PrintModelFrameName(m, e->GetFrame());
}

/*
==================
Host_Viewprev_f
==================
*/
void Host_Viewprev_f(void)
{
	qhedict_t* e;
	qhandle_t m;

	e = FindViewthing();
	if (!e)
	{
		return;
	}

	m = cl.model_draw[(int)e->v.modelindex];

	e->SetFrame(e->GetFrame() - 1);
	if (e->GetFrame() < 0)
	{
		e->SetFrame(0);
	}

	R_PrintModelFrameName(m, e->GetFrame());
}
#endif

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
Host_Startdemos_f
==================
*/
void Host_Startdemos_f(void)
{
#ifndef DEDICATED
	int i, c;

	if (cls.state == CA_DEDICATED)
#endif
	{
		if (sv.state == SS_DEAD)
		{
			Cbuf_AddText("map start\n");
		}
		return;
	}

#ifndef DEDICATED
	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		common->Printf("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	common->Printf("%i demo(s) in loop\n", c);

	for (i = 1; i < c + 1; i++)
		String::NCpy(cls.qh_demos[i - 1], Cmd_Argv(i), sizeof(cls.qh_demos[0]) - 1);

	if (sv.state == SS_DEAD && cls.qh_demonum != -1 && !clc.demoplaying)
	{
		cls.qh_demonum = 0;
		CL_NextDemo();
	}
	else
	{
		cls.qh_demonum = -1;
	}
#endif
}

#ifndef DEDICATED
/*
==================
Host_Demos_f

Return to looping demos
==================
*/
void Host_Demos_f(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}
	if (cls.qh_demonum == -1)
	{
		cls.qh_demonum = 1;
	}
	CL_Disconnect_f();
	CL_NextDemo();
}

/*
==================
Host_Stopdemo_f

Return to looping demos
==================
*/
void Host_Stopdemo_f(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return;
	}
	if (!clc.demoplaying)
	{
		return;
	}
	CL_StopPlayback();
	CL_Disconnect();
}
#endif

//=============================================================================

/*
==================
Host_InitCommands
==================
*/
void Host_InitCommands(void)
{
	SVQH_InitOperatorCommands();
	Cmd_AddCommand("status", Host_Status_f);
	Cmd_AddCommand("quit", Host_Quit_f);
#ifndef DEDICATED
	Cmd_AddCommand("god", Host_God_f);
	Cmd_AddCommand("notarget", Host_Notarget_f);
	Cmd_AddCommand("fly", Host_Fly_f);
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
	Cmd_AddCommand("name", Host_Name_f);
	Cmd_AddCommand("noclip", Host_Noclip_f);
#endif
	Cmd_AddCommand("version", Host_Version_f);
	if (com_dedicated->integer)
	{
		Cmd_AddCommand("say", Host_ConSay_f);
	}
#ifndef DEDICATED
	else
	{
		Cmd_AddCommand("say", Host_Say_f);
		Cmd_AddCommand("say_team", Host_Say_Team_f);
	}
	Cmd_AddCommand("tell", Host_Tell_f);
	Cmd_AddCommand("color", Host_Color_f);
	Cmd_AddCommand("kill", Host_Kill_f);
	Cmd_AddCommand("pause", Host_Pause_f);
#endif
	Cmd_AddCommand("kick", Host_Kick_f);
#ifndef DEDICATED
	Cmd_AddCommand("ping", Host_Ping_f);
	Cmd_AddCommand("give", Host_Give_f);
#endif

	Cmd_AddCommand("startdemos", Host_Startdemos_f);
#ifndef DEDICATED
	Cmd_AddCommand("demos", Host_Demos_f);
	Cmd_AddCommand("stopdemo", Host_Stopdemo_f);

	Cmd_AddCommand("viewmodel", Host_Viewmodel_f);
	Cmd_AddCommand("viewframe", Host_Viewframe_f);
	Cmd_AddCommand("viewnext", Host_Viewnext_f);
	Cmd_AddCommand("viewprev", Host_Viewprev_f);
#endif
}
