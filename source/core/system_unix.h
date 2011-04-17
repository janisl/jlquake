//**************************************************************************
//**
//**	$Id$
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

#ifndef _SYSTEM_UNIX_H
#define _SYSTEM_UNIX_H

void Sys_ConsoleInputInit();
void Sys_ConsoleInputShutdown();
char* Sys_CommonConsoleInput();
void tty_Hide();
void tty_Show();

extern bool				stdin_active;

extern unsigned long	sys_timeBase;

extern QCvar *ttycon;
extern bool		ttycon_on;

#endif
