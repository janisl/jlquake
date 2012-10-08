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

#include "../../client.h"
#include "local.h"

//	Requests a file to download from the server.  Stores it in the current
// game directory.
void CLT3_BeginDownload(const char* localName, const char* remoteName)
{
	common->DPrintf("***** CLT3_BeginDownload *****\n"
				"Localname: %s\n"
				"Remotename: %s\n"
				"****************************\n", localName, remoteName);

	if (GGameType & GAME_ET)
	{
		String::NCpyZ(cls.et_downloadName, localName, sizeof(cls.et_downloadName));
		String::Sprintf(cls.et_downloadTempName, sizeof(cls.et_downloadTempName), "%s.tmp", localName);
	}
	else
	{
		String::NCpyZ(clc.downloadName, localName, sizeof(clc.downloadName));
		String::Sprintf(clc.downloadTempName, sizeof(clc.downloadTempName), "%s.tmp", localName);
	}

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", remoteName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0;	// Starting new file
	clc.downloadCount = 0;

	CL_AddReliableCommand(va("download %s", remoteName));
}
