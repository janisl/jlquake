/*
 * $Header: /H2 Mission Pack/Host_cmd.c 25    4/01/98 4:53p Jmonroe $
 */

#include "quakedef.h"

#ifdef _WIN32
#include <windows.h>
#undef GetClassName
#endif
#include <time.h>
#include "../server/public.h"

/*
==================
Com_Quit_f
==================
*/

void Com_Quit_f(void)
{
#ifndef DEDICATED
	if (!(in_keyCatchers & KEYCATCH_CONSOLE) && !com_dedicated->integer)
	{
		MQH_Menu_Quit_f();
		return;
	}
	CL_Disconnect();
#endif
	SV_Shutdown("");

	Sys_Quit();
}

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
	CL_ClearParticles();	//jfm: for restarts which didn't use to clear parts.

	//updatePlaqueMessage();

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
		CL_ForwardKnownCommandToServer();
	}
}

void Host_Class_f()
{
	if (Cmd_Argc() == 1)
	{
		if (!(int)clh2_playerclass->value)
		{
			common->Printf("\"playerclass\" is %d (\"unknown\")\n", (int)clh2_playerclass->value);
		}
		else
		{
			common->Printf("\"playerclass\" is %d (\"%s\")\n", (int)clh2_playerclass->value,h2_ClassNames[(int)clh2_playerclass->value - 1]);
		}
		return;
	}

	float newClass;
	if (Cmd_Argc() == 2)
	{
		newClass = String::Atof(Cmd_Argv(1));
	}
	else
	{
		newClass = String::Atof(Cmd_ArgsUnmodified());
	}

	Cvar_SetValue("_cl_playerclass", newClass);

	if (GGameType & GAME_H2Portals)
	{
		PR_SetPlayerClassGlobal(newClass);
	}

	if (cls.state == CA_ACTIVE)
	{
		CL_ForwardKnownCommandToServer();
	}
}
#endif

void Host_Version_f(void)
{
	common->Printf("Version %4.2f\n", HEXEN2_VERSION);
	common->Printf("Exe: "__TIME__ " "__DATE__ "\n");
}

#ifndef DEDICATED

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
		CL_ForwardKnownCommandToServer();
	}
}
#endif

//===========================================================================

int strdiff(const char* s1, const char* s2)
{
	int L1,L2,i;

	L1 = String::Length(s1);
	L2 = String::Length(s2);

	for (i = 0; (i < L1 && i < L2); i++)
	{
		if (String::ToLower(s1[i]) != String::ToLower(s2[i]))
		{
			break;
		}
	}

	return i;
}

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
#endif

	if (com_dedicated->integer)
	{
		if (!SV_IsServerActive())
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

	if (!SV_IsServerActive() && cls.qh_demonum != -1 && !clc.demoplaying)
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
	if (com_dedicated->integer)
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
	if (com_dedicated->integer)
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
	Cmd_AddCommand("quit", Com_Quit_f);
#ifndef DEDICATED
	Cmd_AddCommand("god", NULL);
	Cmd_AddCommand("notarget", NULL);
	Cmd_AddCommand("connect", Host_Connect_f);
	Cmd_AddCommand("reconnect", Host_Reconnect_f);
	Cmd_AddCommand("name", Host_Name_f);
	Cmd_AddCommand("playerclass", Host_Class_f);
	Cmd_AddCommand("noclip", NULL);
#endif
	Cmd_AddCommand("version", Host_Version_f);
#ifndef DEDICATED
	Cmd_AddCommand("say", NULL);
	Cmd_AddCommand("say_team", NULL);
	Cmd_AddCommand("tell", NULL);
	Cmd_AddCommand("color", Host_Color_f);
	Cmd_AddCommand("kill", NULL);
	Cmd_AddCommand("pause", NULL);
	Cmd_AddCommand("ping", NULL);
	Cmd_AddCommand("give", NULL);
#endif

	Cmd_AddCommand("startdemos", Host_Startdemos_f);
#ifndef DEDICATED
	Cmd_AddCommand("demos", Host_Demos_f);
	Cmd_AddCommand("stopdemo", Host_Stopdemo_f);
#endif
}
