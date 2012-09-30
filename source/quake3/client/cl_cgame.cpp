/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// cl_cgame.c  -- client system interaction with client game

#include "client.h"
#include "../../client/game/quake3/cg_public.h"

/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified(void)
{
	char* old, * s;
	int i, index;
	char* dup;
	q3gameState_t oldGs;
	int len;

	index = String::Atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_CONFIGSTRINGS_Q3)
	{
		common->Error("configstring > MAX_CONFIGSTRINGS_Q3");
	}
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[index];
	if (!String::Cmp(old, s))
	{
		return;		// unchanged
	}

	// build the new q3gameState_t
	oldGs = cl.q3_gameState;

	Com_Memset(&cl.q3_gameState, 0, sizeof(cl.q3_gameState));

	// leave the first 0 for uninitialized strings
	cl.q3_gameState.dataCount = 1;

	for (i = 0; i < MAX_CONFIGSTRINGS_Q3; i++)
	{
		if (i == index)
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if (!dup[0])
		{
			continue;		// leave with the default empty string
		}

		len = String::Length(dup);

		if (len + 1 + cl.q3_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
		{
			common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
		}

		// append it to the gameState string buffer
		cl.q3_gameState.stringOffsets[i] = cl.q3_gameState.dataCount;
		Com_Memcpy(cl.q3_gameState.stringData + cl.q3_gameState.dataCount, dup, len + 1);
		cl.q3_gameState.dataCount += len + 1;
	}

	if (index == Q3CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}

/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand(int serverCommandNumber)
{
	char* s;
	char* cmd;
	static char bigConfigString[BIG_INFO_STRING];
	int argc;

	// if we have irretrievably lost a reliable command, drop the connection
	if (serverCommandNumber <= clc.q3_serverCommandSequence - MAX_RELIABLE_COMMANDS_Q3)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if (clc.demoplaying)
		{
			return false;
		}
		common->Error("CL_GetServerCommand: a reliable command was cycled out");
		return false;
	}

	if (serverCommandNumber > clc.q3_serverCommandSequence)
	{
		common->Error("CL_GetServerCommand: requested a command not received");
		return false;
	}

	s = clc.q3_serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS_Q3 - 1)];
	clc.q3_lastExecutedServerCommand = serverCommandNumber;

	common->DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);

rescan:
	Cmd_TokenizeString(s);
	cmd = Cmd_Argv(0);
	argc = Cmd_Argc();

	if (!String::Cmp(cmd, "disconnect"))
	{
		// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=552
		// allow server to indicate why they were disconnected
		if (argc >= 2)
		{
			Com_Error(ERR_SERVERDISCONNECT, va("Server Disconnected - %s", Cmd_Argv(1)));
		}
		else
		{
			Com_Error(ERR_SERVERDISCONNECT,"Server disconnected\n");
		}
	}

	if (!String::Cmp(cmd, "bcs0"))
	{
		String::Sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return false;
	}

	if (!String::Cmp(cmd, "bcs1"))
	{
		s = Cmd_Argv(2);
		if (String::Length(bigConfigString) + String::Length(s) >= BIG_INFO_STRING)
		{
			common->Error("bcs exceeded BIG_INFO_STRING");
		}
		String::Cat(bigConfigString, sizeof(bigConfigString), s);
		return false;
	}

	if (!String::Cmp(cmd, "bcs2"))
	{
		s = Cmd_Argv(2);
		if (String::Length(bigConfigString) + String::Length(s) + 1 >= BIG_INFO_STRING)
		{
			common->Error("bcs exceeded BIG_INFO_STRING");
		}
		String::Cat(bigConfigString, sizeof(bigConfigString), s);
		String::Cat(bigConfigString, sizeof(bigConfigString), "\"");
		s = bigConfigString;
		goto rescan;
	}

	if (!String::Cmp(cmd, "cs"))
	{
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return true;
	}

	if (!String::Cmp(cmd, "map_restart"))
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		Com_Memset(cl.q3_cmds, 0, sizeof(cl.q3_cmds));
		return true;
	}

	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if (!String::Cmp(cmd, "clientLevelShot"))
	{
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if (!com_sv_running->integer)
		{
			return false;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");
		return true;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return true;
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
qintptr CL_CgameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
//---------
	case Q3CG_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
//---------
	case Q3CG_GETSERVERCOMMAND:
		return CL_GetServerCommand(args[1]);
//---------
	}
	return CLQ3_CgameSystemCalls(args);
}


/*
====================
CL_InitCGame

Should only be called by CL_StartHunkUsers
====================
*/
void CL_InitCGame(void)
{
	const char* info;
	const char* mapname;
	int t1, t2;
	vmInterpret_t interpret;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[Q3CS_SERVERINFO];
	mapname = Info_ValueForKey(info, "mapname");
	String::Sprintf(cl.q3_mapname, sizeof(cl.q3_mapname), "maps/%s.bsp", mapname);

	// load the dll or bytecode
	if (cl_connectedToPureServer != 0)
	{
		// if sv_pure is set we only allow qvms to be loaded
		interpret = VMI_COMPILED;
	}
	else
	{
		interpret = (vmInterpret_t)(int)Cvar_VariableValue("vm_cgame");
	}
	cgvm = VM_Create("cgame", CL_CgameSystemCalls, interpret);
	if (!cgvm)
	{
		common->Error("VM_Create on cgame failed");
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call(cgvm, CG_INIT, clc.q3_serverMessageSequence, clc.q3_lastExecutedServerCommand, clc.q3_clientNum);

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	common->Printf("CL_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	R_EndRegistration();

	// clear anything that got printed
	Con_ClearNotify();
}

/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME  500

void CL_AdjustTimeDelta(void)
{
	int resetTime;
	int newDelta;
	int deltaDelta;

	cl.q3_newSnapshots = false;

	// the delta never drifts when replaying a demo
	if (clc.demoplaying)
	{
		return;
	}

	// if the current time is WAY off, just correct to the current value
	if (com_sv_running->integer)
	{
		resetTime = 100;
	}
	else
	{
		resetTime = RESET_TIME;
	}

	newDelta = cl.q3_snap.serverTime - cls.realtime;
	deltaDelta = abs(newDelta - cl.q3_serverTimeDelta);

	if (deltaDelta > RESET_TIME)
	{
		cl.q3_serverTimeDelta = newDelta;
		cl.q3_oldServerTime = cl.q3_snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.q3_snap.serverTime;
		if (cl_showTimeDelta->integer)
		{
			common->Printf("<RESET> ");
		}
	}
	else if (deltaDelta > 100)
	{
		// fast adjust, cut the difference in half
		if (cl_showTimeDelta->integer)
		{
			common->Printf("<FAST> ");
		}
		cl.q3_serverTimeDelta = (cl.q3_serverTimeDelta + newDelta) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if (com_timescale->value == 0 || com_timescale->value == 1)
		{
			if (cl.q3_extrapolatedSnapshot)
			{
				cl.q3_extrapolatedSnapshot = false;
				cl.q3_serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.q3_serverTimeDelta++;
			}
		}
	}

	if (cl_showTimeDelta->integer)
	{
		common->Printf("%i ", cl.q3_serverTimeDelta);
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot(void)
{
	// ignore snapshots that don't have entities
	if (cl.q3_snap.snapFlags & Q3SNAPFLAG_NOT_ACTIVE)
	{
		return;
	}
	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.q3_serverTimeDelta = cl.q3_snap.serverTime - cls.realtime;
	cl.q3_oldServerTime = cl.q3_snap.serverTime;

	clc.q3_timeDemoBaseTime = cl.q3_snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if (cl_activeAction->string[0])
	{
		Cbuf_AddText(cl_activeAction->string);
		Cvar_Set("activeAction", "");
	}

	Sys_BeginProfiling();
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime(void)
{
	// getting a valid frame message ends the connection process
	if (cls.state != CA_ACTIVE)
	{
		if (cls.state != CA_PRIMED)
		{
			return;
		}
		if (clc.demoplaying)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if (!clc.q3_firstDemoFrameSkipped)
			{
				clc.q3_firstDemoFrameSkipped = true;
				return;
			}
			CL_ReadDemoMessage();
		}
		if (cl.q3_newSnapshots)
		{
			cl.q3_newSnapshots = false;
			CL_FirstSnapshot();
		}
		if (cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.snap is guaranteed to be valid
	if (!cl.q3_snap.valid)
	{
		common->Error("CL_SetCGameTime: !cl.snap.valid");
	}

	// allow pause in single player
	if (sv_paused->integer && cl_paused->integer && com_sv_running->integer)
	{
		// paused
		return;
	}

	if (cl.q3_snap.serverTime < cl.q3_oldFrameServerTime)
	{
		common->Error("cl.snap.serverTime < cl.oldFrameServerTime");
	}
	cl.q3_oldFrameServerTime = cl.q3_snap.serverTime;


	// get our current view of time

	if (clc.demoplaying && cl_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = cl_timeNudge->integer;
		if (tn < -30)
		{
			tn = -30;
		}
		else if (tn > 30)
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.q3_serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if (cl.serverTime < cl.q3_oldServerTime)
		{
			cl.serverTime = cl.q3_oldServerTime;
		}
		cl.q3_oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if (cls.realtime + cl.q3_serverTimeDelta >= cl.q3_snap.serverTime - 5)
		{
			cl.q3_extrapolatedSnapshot = true;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if (cl.q3_newSnapshots)
	{
		CL_AdjustTimeDelta();
	}

	if (!clc.demoplaying)
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if (cl_timedemo->integer)
	{
		if (!clc.q3_timeDemoStart)
		{
			clc.q3_timeDemoStart = Sys_Milliseconds();
		}
		clc.q3_timeDemoFrames++;
		cl.serverTime = clc.q3_timeDemoBaseTime + clc.q3_timeDemoFrames * 50;
	}

	while (cl.serverTime >= cl.q3_snap.serverTime)
	{
		// feed another messag, which should change
		// the contents of cl.snap
		CL_ReadDemoMessage();
		if (cls.state != CA_ACTIVE)
		{
			return;		// end of demo
		}
	}

}
