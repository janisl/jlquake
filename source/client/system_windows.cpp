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
#include "windows_shared.h"
#include "../common/Common.h"
#include "../common/command_buffer.h"
#include "../common/strings.h"

HWND GMainWindow;
bool ActiveApp;

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

void Sys_StartProcess(const char* exeName, bool doExit)
{
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	TCHAR szPathOrig[_MAX_PATH];
	GetCurrentDirectory(_MAX_PATH, szPathOrig);

	PROCESS_INFORMATION pi;
	if (!CreateProcess(NULL, va("%s\\%s", szPathOrig, exeName), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		// couldn't start it, popup error box
		common->Error("Could not start process: '%s\\%s' ", szPathOrig, exeName);
		return;
	}

	// TTimo: similar way of exiting as used in Sys_OpenURL below
	if (doExit)
	{
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
	}
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

	if (!ShellExecute(NULL, "open", url, NULL, NULL, SW_RESTORE))
	{
		// couldn't start it, popup error box
		common->Error("Could not open url: '%s' ", url);
		return;
	}

	HWND wnd = GetForegroundWindow();

	if (wnd)
	{
		ShowWindow(wnd, SW_MAXIMIZE);
	}

	if (doexit)
	{
		doexit_spamguard = true;
		Cbuf_ExecuteText(EXEC_APPEND, "quit\n");
	}
}

bool Sys_IsNumLockDown()
{
	// thx juz ;)
	SHORT state = GetKeyState(VK_NUMLOCK);

	if (state & 0x01)
	{
		return true;
	}

	return false;
}

void Sys_SendKeyEvents()
{
}

void Sys_AppActivate()
{
	ShowWindow(GMainWindow, SW_RESTORE);
	SetForegroundWindow(GMainWindow);
}
