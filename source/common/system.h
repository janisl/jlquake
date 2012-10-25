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

int Sys_StatFile(const char* ospath);
void Sys_Mkdir(const char* path);
int Sys_Rmdir(const char* path);
const char* Sys_Cwd();
void Sys_SetHomePathSuffix(const char* Name);
const char* Sys_DefaultHomePath();

char** Sys_ListFiles(const char* directory, const char* extension, const char* filter, int* numfiles, bool wantsubs);
void Sys_FreeFileList(char** list);

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int Sys_Milliseconds();
double Sys_DoubleTime();

void Sys_ShowConsole(int level, bool quitOnClose);
char* Sys_ConsoleInput();
void Sys_Print(const char* msg);

bool Sys_Quake3DllWarningConfirmation();
char* Sys_GetDllName(const char* name);
char* Sys_GetMPDllName(const char* name);
void* Sys_LoadDll(const char* name);
void* Sys_GetDllFunction(void* handle, const char* name);
void Sys_UnloadDll(void* handle);

void Sys_MessageLoop();
void Sys_Quit();
void Sys_Error(const char* error, ...);
const char* Sys_GetCurrentUser();
void Sys_Init();
