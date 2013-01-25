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

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "../../common/qcommon.h"
#include "input.h"

#define CAM_NONE    0
#define CAM_TRACK   1

extern int spec_track;	// player# of who we are tracking
extern int autocam;

void CL_InitCam();
void Cam_Reset();
void Cam_Track( in_usercmd_t* cmd );
void Cam_FinishMove( const in_usercmd_t* cmd );
bool Cam_DrawViewModel();
bool Cam_DrawPlayer( int playernum );

#endif
