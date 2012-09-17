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

#include "../common/qcommon.h"
#include "../client/public.h"

void S_ClearSoundBuffer(bool killStreaming)
{
}

int CLH2_GetLightStyleValue(int style)
{
	return 0;
}

void CL_ClearDrift()
{
}

int CL_GetKeyCatchers()
{
	return 0;
}

void CLQH_StopDemoLoop()
{
}

void CL_ClearKeyCatchers()
{
}

void CLQH_GetSpawnParams()
{
}

bool CL_IsDemoPlaying()
{
	return false;
}

int CLQH_GetIntermission()
{
	return 0;
}

void SCR_DebugGraph(float value, int color)
{
}

void CL_CvarChanged(Cvar* var)
{
}

const char* CL_TranslateStringBuf(const char* string)
{
	return string;
}

void Key_WriteBindings(fileHandle_t f)
{
}

void CL_InitKeyCommands()
{
}

bool CLT3_GameCommand()
{
	return false;
}

bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	return false;
}

bool UIT3_GameCommand()
{
	return false;
}
