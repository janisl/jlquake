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

//
//	These constants are used by Quake 3 VMs, do not change.
//

// cinematic states
enum e_status
{
	FMV_IDLE,
	FMV_PLAY,		// play
	FMV_EOF,		// all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
};

enum
{
	CIN_system		= BIT(0),
	CIN_loop		= BIT(1),
	CIN_hold		= BIT(2),
	CIN_silent		= BIT(3),
	CIN_shader		= BIT(4),
	CIN_letterBox	= BIT(5),
};

int CIN_PlayCinematic(const char* name, int xPos, int yPos, int width, int height, int bits);
void CIN_SetExtents(int handle, int x, int y, int w, int h);
e_status CIN_RunCinematic(int handle);
void CIN_UploadCinematic(int handle);
void CIN_DrawCinematic(int handle);
e_status CIN_StopCinematic(int handle);
void SCR_PlayCinematic(const char* name);
void SCR_RunCinematic();
bool SCR_DrawCinematic();
void SCR_StopCinematic();
void CIN_SkipCinematic();
void CIN_CloseAllVideos();
void CL_PlayCinematic_f();
