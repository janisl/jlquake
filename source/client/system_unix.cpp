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

#include "system.h"
#include "unix_shared.h"
#include <unistd.h>
#include <errno.h>

char* Sys_GetClipboardData()
{
	return NULL;
}

//	if !doexit, start the process asap
// otherwise, push it for execution at exit
// (i.e. let complete shutdown of the game and freeing of resources happen)
// NOTE: might even want to add a small delay?
void Sys_StartProcess(const char* cmdline, bool doexit)
{
	if (doexit)
	{
		common->DPrintf("Sys_StartProcess %s (delaying to final exit)\n", cmdline);
		String::NCpyZ(exit_cmdline, cmdline, MAX_CMD);
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
		return;
	}

	common->DPrintf("Sys_StartProcess %s\n", cmdline);
	Sys_DoStartProcess(cmdline);
}

void Sys_OpenURL(const char* url, bool doexit)
{
	static bool doexit_spamguard = false;

	if (doexit_spamguard)
	{
		common->DPrintf("Sys_OpenURL: already in a doexit sequence, ignoring %s\n", url);
		return;
	}

	common->Printf("Open URL: %s\n", url);
	// opening an URL on *nix can mean a lot of things ..
	// just spawn a script instead of deciding for the user :-)

	// do the setup before we fork
	// search for an openurl.sh script
	// search procedure taken from Sys_VM_LoadDll
	char fname[20];
	String::NCpyZ(fname, "openurl.sh", 20);

	const char* pwdpath = Sys_Cwd();
	char fn[MAX_OSPATH];
	String::Sprintf(fn, MAX_OSPATH, "%s/%s", pwdpath, fname);
	if (access(fn, X_OK) == -1)
	{
		common->DPrintf("%s not found\n", fn);
		// try in home path
		const char* homepath = Cvar_VariableString("fs_homepath");
		String::Sprintf(fn, MAX_OSPATH, "%s/%s", homepath, fname);
		if (access(fn, X_OK) == -1)
		{
			common->DPrintf("%s not found\n", fn);
			// basepath, last resort
			const char* basepath = Cvar_VariableString("fs_basepath");
			String::Sprintf(fn, MAX_OSPATH, "%s/%s", basepath, fname);
			if (access(fn, X_OK) == -1)
			{
				common->DPrintf("%s not found\n", fn);
				common->Printf("Can't find script '%s' to open requested URL (use +set developer 1 for more verbosity)\n", fname);
				// we won't quit
				return;
			}
		}
	}

	if (doexit)
	{
		doexit_spamguard = true;
	}

	common->DPrintf("URL script: %s\n", fn);

	// build the command line
	char cmdline[MAX_CMD];
	String::Sprintf(cmdline, MAX_CMD, "%s '%s' &", fn, url);

	Sys_StartProcess(cmdline, doexit);
}

bool Sys_IsNumLockDown()
{
	// Gordon: FIXME for timothee
	return false;
}

void Sys_AppActivate()
{
}
