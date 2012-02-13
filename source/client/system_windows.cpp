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

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "windows_shared.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

HWND			GMainWindow;
bool			Minimized;
bool			ActiveApp;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Sys_GetClipboardData
//
//==========================================================================

char* Sys_GetClipboardData()
{
	char* data = NULL;

	if (OpenClipboard(NULL))
	{
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		if (hClipboardData)
		{
			char* clipText = (char*)GlobalLock(hClipboardData);
			if (clipText)
			{
				data = new char[GlobalSize(hClipboardData) + 1];
				String::NCpyZ(data, clipText, GlobalSize(hClipboardData));
				GlobalUnlock(hClipboardData);
				
				strtok(data, "\n\r\b");
			}
		}
		CloseClipboard();
	}
	return data;
}
