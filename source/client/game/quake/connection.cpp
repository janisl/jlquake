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

//	An q1svc_signonnum has been received, perform a client side setup
void CLQ1_SignonReply()
{
	char str[8192];

	common->DPrintf("CLQ1_SignonReply: %i\n", clc.qh_signon);

	switch (clc.qh_signon)
	{
	case 1:
		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2("prespawn");
		break;

	case 2:
		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2(va("name \"%s\"\n", clqh_name->string));

		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2(va("color %i %i\n", clqh_color->integer >> 4, clqh_color->integer & 15));

		clc.netchan.message.WriteByte(q1clc_stringcmd);
		sprintf(str, "spawn %s", cls.qh_spawnparms);
		clc.netchan.message.WriteString2(str);
		break;

	case 3:
		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2("begin");
		break;

	case 4:
		SCR_EndLoadingPlaque();		// allow normal screen updates
		break;
	}
}

//	Returns true if the file exists, otherwise it attempts
// to start a download from the server.
bool CLQW_CheckOrDownloadFile(const char* filename)
{
	fileHandle_t f;

	if (strstr(filename, ".."))
	{
		common->Printf("Refusing to download a path with ..\n");
		return true;
	}

	FS_FOpenFileRead(filename, &f, true);
	if (f)
	{
		// it exists, no need to download
		FS_FCloseFile(f);
		return true;
	}

	//ZOID - can't download when recording
	if (clc.demorecording)
	{
		common->Printf("Unable to download %s in record mode.\n", clc.downloadName);
		return true;
	}
	//ZOID - can't download when playback
	if (clc.demoplaying)
	{
		return true;
	}

	String::Cpy(clc.downloadName, filename);
	common->Printf("Downloading %s...\n", clc.downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc.downloadName, clc.downloadTempName);
	String::Cat(clc.downloadTempName, sizeof(clc.downloadTempName), ".tmp");

	clc.netchan.message.WriteByte(q1clc_stringcmd);
	clc.netchan.message.WriteString2(va("download %s", clc.downloadName));

	clc.downloadNumber++;

	return false;
}
