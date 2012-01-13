//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
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

	Log::develWrite("CLQ1_SignonReply: %i\n", clc_common->qh_signon);

	switch (clc_common->qh_signon)
	{
	case 1:
		clc_common->netchan.message.WriteByte(q1clc_stringcmd);
		clc_common->netchan.message.WriteString2("prespawn");
		break;
		
	case 2:		
		clc_common->netchan.message.WriteByte(q1clc_stringcmd);
		clc_common->netchan.message.WriteString2(va("name \"%s\"\n", clqh_name->string));
	
		clc_common->netchan.message.WriteByte(q1clc_stringcmd);
		clc_common->netchan.message.WriteString2(va("color %i %i\n", clqh_color->integer >> 4, clqh_color->integer & 15));
	
		clc_common->netchan.message.WriteByte(q1clc_stringcmd);
		sprintf(str, "spawn %s", cls_common->qh_spawnparms);
		clc_common->netchan.message.WriteString2(str);
		break;
		
	case 3:	
		clc_common->netchan.message.WriteByte(q1clc_stringcmd);
		clc_common->netchan.message.WriteString2("begin");
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
	fileHandle_t	f;

	if (strstr(filename, ".."))
	{
		Log::write("Refusing to download a path with ..\n");
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
	if (clc_common->demorecording)
	{
		Log::write("Unable to download %s in record mode.\n", clc_common->downloadName);
		return true;
	}
	//ZOID - can't download when playback
	if (clc_common->demoplaying)
	{
		return true;
	}

	String::Cpy(clc_common->downloadName, filename);
	Log::write("Downloading %s...\n", clc_common->downloadName);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	String::StripExtension(clc_common->downloadName, clc_common->downloadTempName);
	String::Cat(clc_common->downloadTempName, sizeof(clc_common->downloadTempName), ".tmp");

	clc_common->netchan.message.WriteByte(q1clc_stringcmd);
	clc_common->netchan.message.WriteString2(va("download %s", clc_common->downloadName));

	clc_common->downloadNumber++;

	return false;
}
