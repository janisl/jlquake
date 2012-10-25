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

#ifndef _SYSTEM_UNIX_H
#define _SYSTEM_UNIX_H

#define MAX_CMD 1024

void Sys_DoStartProcess(const char* cmdline);
void Sys_Exit(int ex);
void InitSig();

void Sys_ConsoleInputInit();
void Sys_ConsoleInputShutdown();
void tty_Hide();
void tty_Show();

extern bool stdin_active;

extern unsigned long sys_timeBase;

extern char exit_cmdline[MAX_CMD];

extern Cvar* ttycon;
extern bool ttycon_on;

#endif
