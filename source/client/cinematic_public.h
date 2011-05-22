//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
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

#define CIN_system	1
#define CIN_loop	2
#define CIN_hold	4
#define CIN_silent	8
#define CIN_shader	16

int CIN_PlayCinematic(const char* Name, int XPos, int YPos, int Width, int Height, int Bits);
e_status CIN_RunCinematic(int Handle);
void CIN_UploadCinematic(int Handle);

//	callbacks
void CIN_StartedPlayback();
bool CIN_IsInCinematicState();
void CIN_FinishCinematic();
