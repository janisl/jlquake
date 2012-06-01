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

#include "../server.h"
#include "local.h"
#include "ai_weight.h"

bot_debugpoly_t* debugpolygons;
int bot_maxdebugpolys;

int bot_developer;

//library globals in a structure
botlib_globals_t botlibglobals;

//true if the library is setup
static bool botlibsetup = false;

void BotImport_Print(int type, const char* fmt, ...)
{
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch (type)
	{
	case PRT_MESSAGE:
		common->Printf("%s", str);
		break;

	case PRT_WARNING:
		common->Printf(S_COLOR_YELLOW "Warning: %s", str);
		break;

	case PRT_ERROR:
		common->Printf(S_COLOR_RED "Error: %s", str);
		break;

	case PRT_FATAL:
		common->Printf(S_COLOR_RED "Fatal: %s", str);
		break;

	case PRT_EXIT:
		common->Error(S_COLOR_RED "Exit: %s", str);
		break;

	default:
		common->Printf("unknown print type\n");
		break;
	}
}

int BotImport_DebugPolygonCreate(int color, int numPoints, const vec3_t* points)
{
	if (!debugpolygons)
	{
		return 0;
	}

	int i;
	for (i = 1; i < bot_maxdebugpolys; i++)
	{
		if (!debugpolygons[i].inuse)
		{
			break;
		}
	}
	if (i >= bot_maxdebugpolys)
	{
		return 0;
	}
	bot_debugpoly_t* poly = &debugpolygons[i];
	poly->inuse = true;
	poly->color = color;
	poly->numPoints = numPoints;
	Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));

	return i;
}

void BotImport_DebugPolygonDelete(int id)
{
	if (!debugpolygons)
	{
		return;
	}
	debugpolygons[id].inuse = false;
}

int BotImport_DebugLineCreate()
{
	vec3_t points[1];
	return BotImport_DebugPolygonCreate(0, 0, points);
}

void BotImport_DebugLineDelete(int line)
{
	BotImport_DebugPolygonDelete(line);
}

static void BotImport_DebugPolygonShow(int id, int color, int numPoints, const vec3_t* points)
{
	bot_debugpoly_t* poly;

	if (!debugpolygons)
	{
		return;
	}
	poly = &debugpolygons[id];
	poly->inuse = true;
	poly->color = color;
	poly->numPoints = numPoints;
	Com_Memcpy(poly->points, points, numPoints * sizeof(vec3_t));
}

void BotImport_DebugLineShow(int line, const vec3_t start, const vec3_t end, int color)
{
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy(start, points[0]);
	VectorCopy(start, points[1]);
	//points[1][2] -= 2;
	VectorCopy(end, points[2]);
	//points[2][2] -= 2;
	VectorCopy(end, points[3]);


	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99 || dot < -0.99)
	{
		VectorSet(cross, 1, 0, 0);
	}
	else
	{
		CrossProduct(dir, up, cross);
	}

	VectorNormalize(cross);

	VectorMA(points[0], 2, cross, points[0]);
	VectorMA(points[1], -2, cross, points[1]);
	VectorMA(points[2], -2, cross, points[2]);
	VectorMA(points[3], 2, cross, points[3]);

	BotImport_DebugPolygonShow(line, color, 4, points);
}

static bool ValidEntityNumber(int num, const char* str)
{
	if (num < 0 || num > botlibglobals.maxentities)
	{
		BotImport_Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n",
			str, num, botlibglobals.maxentities);
		return false;
	}
	return true;
}

bool IsBotLibSetup(const char* str)
{
	if (!botlibglobals.botlibsetup)
	{
		BotImport_Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
		return false;
	}
	return true;
}

int BotLibSetup(bool singleplayer)
{
	bot_developer = LibVarGetValue("bot_developer");
	Com_Memset(&botlibglobals, 0, sizeof(botlibglobals));
	Log_Open("botlib.log");

	BotImport_Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");

	botlibglobals.maxclients = (int)LibVarValue("maxclients", "128");
	botlibglobals.maxentities = (int)LibVarValue("maxentities", GGameType & GAME_WolfSP ? "2048" : "1024");

	int errnum = AAS_Setup();			//be_aas_main.c
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}
	errnum = EA_Setup();			//be_ea.c
	if (errnum != BLERR_NOERROR)
	{
		return errnum;
	}
	if (!(GGameType & (GAME_WolfSP | GAME_WolfMP)))
	{
		errnum = BotSetupWeaponAI();	//be_ai_weap.c
		if (errnum != BLERR_NOERROR)
		{
			return errnum;
		}
		errnum = BotSetupGoalAI(singleplayer);		//be_ai_goal.c
		if (errnum != BLERR_NOERROR)
		{
			return errnum;
		}
		errnum = BotSetupChatAI();		//be_ai_chat.c
		if (errnum != BLERR_NOERROR)
		{
			return errnum;
		}
	}
	if (!(GGameType & GAME_WolfMP))
	{
		errnum = BotSetupMoveAI();		//be_ai_move.c
		if (errnum != BLERR_NOERROR)
		{
			return errnum;
		}
	}

	if (GGameType & GAME_ET)
	{
		PC_RemoveAllGlobalDefines();
	}

	botlibsetup = true;
	botlibglobals.botlibsetup = true;

	return BLERR_NOERROR;
}

int BotLibShutdown()
{
	if (!IsBotLibSetup("BotLibShutdown"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}

	// shutdown all AI subsystems
	BotShutdownChatAI();		//be_ai_chat.c
	BotShutdownMoveAI();		//be_ai_move.c
	BotShutdownGoalAI();		//be_ai_goal.c
	BotShutdownWeaponAI();		//be_ai_weap.c
	BotShutdownWeights();		//be_ai_weight.c
	BotShutdownCharacters();	//be_ai_char.c
	// shut down aas
	AAS_Shutdown();
	// shut down bot elemantary actions
	EA_Shutdown();
	// free all libvars
	LibVarDeAllocAll();
	// remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();

	// shut down library log file
	Log_Shutdown();

	botlibsetup = false;
	botlibglobals.botlibsetup = false;
	// print any files still open
	PC_CheckOpenSourceHandles();
	return BLERR_NOERROR;
}

int BotLibUpdateEntity(int ent, bot_entitystate_t* state)
{
	if (!IsBotLibSetup("BotUpdateEntity"))
	{
		return BLERR_LIBRARYNOTSETUP;
	}
	if (!ValidEntityNumber(ent, "BotUpdateEntity"))
	{
		return GGameType & GAME_Quake3 ? Q3BLERR_INVALIDENTITYNUMBER : WOLFBLERR_INVALIDENTITYNUMBER;
	}

	return AAS_UpdateEntity(ent, state);
}
