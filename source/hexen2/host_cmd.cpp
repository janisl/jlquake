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
#include "../client/game/quake_hexen2/demo.h"
#include "../client/game/quake_hexen2/connection.h"

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
	CL_Disconnect(true);
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
	Cmd_AddCommand("connect", CLQH_Connect_f);
	Cmd_AddCommand("reconnect", CLQH_Reconnect_f);
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

	Cmd_AddCommand("startdemos", CLQH_Startdemos_f);
	Cmd_AddCommand("demos", CLQH_Demos_f);
	Cmd_AddCommand("stopdemo", CLQH_Stopdemo_f);
#endif
}
