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

#ifndef _TECH3_CG_SHARED_H
#define _TECH3_CG_SHARED_H

enum
{
	CGAME_EVENT_NONE
};

/*
==================================================================

functions exported to the main executable

==================================================================
*/

enum
{
	CG_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)();
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)();
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return false if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)();

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)();

	CG_KEY_EVENT,
//	void (*CG_KeyEvent)(int key, qboolean down);

	CG_MOUSE_EVENT,
//	void (*CG_MouseEvent)( int dx, int dy );

	CG_EVENT_HANDLING
//	void (*CG_EventHandling)(int type);
};

extern vm_t* cgvm;				// interface to cgame dll or vm

#endif
