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

#include "client.h"
#include "../server/public.h"

Cvar* cl_inGameVideo;

Cvar* clqh_nolerp;

// these two are not intended to be set directly
Cvar* clqh_name;
Cvar* clqh_color;

clientActive_t cl;
clientConnection_t clc;
clientStatic_t cls;

byte* playerTranslation;

int color_offsets[MAX_PLAYER_CLASS] =
{
	2 * 14 * 256,
	0,
	1 * 14 * 256,
	2 * 14 * 256,
	2 * 14 * 256,
	2 * 14 * 256
};

int bitcounts[32];	/// just for protocol profiling

float clqh_server_version = 0;	// version of server we connected to

static void CL_ForwardToServer_f()
{
	if (cls.state != CA_ACTIVE || clc.demoplaying)
	{
		common->Printf("Not connected to a server.\n");
		return;
	}

	if (GGameType & GAME_QuakeWorld && String::ICmp(Cmd_Argv(1), "snap") == 0)
	{
		Cbuf_InsertText("snap\n");
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		CL_AddReliableCommand(GGameType & GAME_Tech3 ? Cmd_Args() : Cmd_ArgsUnmodified());
	}
}

void CL_SharedInit()
{
	cl_inGameVideo = Cvar_Get("r_inGameVideo", "1", CVAR_ARCHIVE);
	cl_shownet = Cvar_Get("cl_shownet", "0", CVAR_TEMP);

	//
	// register our commands
	//
	Cmd_AddCommand("cmd", CL_ForwardToServer_f);
}

int CL_ScaledMilliseconds()
{
	return Sys_Milliseconds() * com_timescale->value;
}

void CL_CalcQuakeSkinTranslation(int top, int bottom, byte* translate)
{
	enum
	{
		// soldier uniform colors
		TOP_RANGE = 16,
		BOTTOM_RANGE = 96
	};

	top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
	bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
	top *= 16;
	bottom *= 16;

	for (int i = 0; i < 256; i++)
	{
		translate[i] = i;
	}

	for (int i = 0; i < 16; i++)
	{
		//	The artists made some backwards ranges. sigh.
		if (top < 128)
		{
			translate[TOP_RANGE + i] = top + i;
		}
		else
		{
			translate[TOP_RANGE + i] = top + 15 - i;
		}

		if (bottom < 128)
		{
			translate[BOTTOM_RANGE + i] = bottom + i;
		}
		else
		{
			translate[BOTTOM_RANGE + i] = bottom + 15 - i;
		}
	}
}

void CL_CalcHexen2SkinTranslation(int top, int bottom, int playerClass, byte* translate)
{
	for (int i = 0; i < 256; i++)
	{
		translate[i] = i;
	}

	if (top > 10)
	{
		top = 0;
	}
	if (bottom > 10)
	{
		bottom = 0;
	}

	top -= 1;
	bottom -= 1;

	byte* colorA = playerTranslation + 256 + color_offsets[playerClass - 1];
	byte* colorB = colorA + 256;
	byte* sourceA = colorB + 256 + top * 256;
	byte* sourceB = colorB + 256 + bottom * 256;
	for (int i = 0; i < 256; i++, colorA++, colorB++, sourceA++, sourceB++)
	{
		if (top >= 0 && *colorA != 255)
		{
			translate[i] = *sourceA;
		}
		if (bottom >= 0 && *colorB != 255)
		{
			translate[i] = *sourceB;
		}
	}
}

//	Determines the fraction between the last two messages that the objects
// should be put at.
float CLQH_LerpPoint()
{
	float f = cl.qh_mtime[0] - cl.qh_mtime[1];

	if (!f || clqh_nolerp->value || cls.qh_timedemo || SV_IsServerActive())
	{
		cl.qh_serverTimeFloat = cl.qh_mtime[0];
		cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
		return 1;
	}

	if (f > 0.1)
	{
		// dropped packet, or start of demo
		cl.qh_mtime[1] = cl.qh_mtime[0] - 0.1;
		f = 0.1;
	}
	float frac = (cl.qh_serverTimeFloat - cl.qh_mtime[1]) / f;
	if (frac < 0)
	{
		if (frac < -0.01)
		{
			cl.qh_serverTimeFloat = cl.qh_mtime[1];
			cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
		}
		frac = 0;
	}
	else if (frac > 1)
	{
		if (frac > 1.01)
		{
			cl.qh_serverTimeFloat = cl.qh_mtime[0];
			cl.serverTime = (int)(cl.qh_serverTimeFloat * 1000);
		}
		frac = 1;
	}
	return frac;
}

void CL_ClearDrift()
{
	cl.qh_nodrift = false;
	cl.qh_driftmove = 0;
}

void CLQH_StopDemoLoop()
{
	cls.qh_demonum = -1;
}

void CL_ClearKeyCatchers()
{
	in_keyCatchers = 0;
}

void CLQH_GetSpawnParams()
{
	String::Cpy(cls.qh_spawnparms, "");

	for (int i = 2; i < Cmd_Argc(); i++)
	{
		String::Cat(cls.qh_spawnparms, sizeof(cls.qh_spawnparms), Cmd_Argv(i));
		String::Cat(cls.qh_spawnparms, sizeof(cls.qh_spawnparms), " ");
	}
}

bool CL_IsDemoPlaying()
{
	return clc.demoplaying;
}

int CLQH_GetIntermission()
{
	return cl.qh_intermission;
}

//	The given command will be transmitted to the server, and is gauranteed to
// not have future q3usercmd_t executed before it is executed
void CL_AddReliableCommand(const char* cmd)
{
	if (GGameType & GAME_Tech3)
	{
		// if we would be losing an old command that hasn't been acknowledged,
		// we must drop the connection
		int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
		if (clc.q3_reliableSequence - clc.q3_reliableAcknowledge > maxReliableCommands)
		{
			common->Error("Client command overflow");
		}
		clc.q3_reliableSequence++;
		int index = clc.q3_reliableSequence & (maxReliableCommands - 1);
		String::NCpyZ(clc.q3_reliableCommands[index], cmd, sizeof(clc.q3_reliableCommands[index]));
	}
	else
	{
		clc.netchan.message.WriteByte(GGameType & GAME_Quake ? q1clc_stringcmd :
			GGameType & GAME_Hexen2 ? h2clc_stringcmd : q2clc_stringcmd);
		clc.netchan.message.WriteString2(cmd);
	}
}

void CL_CvarChanged(Cvar* var)
{
	if (!(GGameType & (GAME_QuakeWorld | GAME_HexenWorld)))
	{
		return;
	}

	if (var->flags & CVAR_USERINFO && var->name[0] != '*')
	{
		Info_SetValueForKey(cls.qh_userinfo, var->name, var->string, MAX_INFO_STRING_QW, 64, 64,
			String::ICmp(var->name, "name") != 0, String::ICmp(var->name, "team") == 0);
		if (cls.state == CA_CONNECTED || cls.state == CA_LOADING || cls.state == CA_ACTIVE)
		{
			CL_AddReliableCommand(va("setinfo \"%s\" \"%s\"\n", var->name, var->string));
		}
	}
}
