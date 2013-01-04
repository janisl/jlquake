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

#ifndef __COMMAND_LINE_ARGS_H__
#define __COMMAND_LINE_ARGS_H__

#include "qcommon.h"

int COM_Argc();
const char* COM_Argv(int arg);	// range and null checked
void COM_InitArgv(int argc, char** argv);
void COM_AddParm(const char* parm);
void COM_InitArgv2(int argc, char** argv);
void COM_ClearArgv(int arg);
int COM_CheckParm(const char* parm);

void Com_ParseCommandLine(char* commandLine);
bool Com_SafeMode();
bool Com_AddStartupCommands();
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.
void Cmd_StuffCmds_f();
void Com_StartupVariable(const char* match);

#endif

