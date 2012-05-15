/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		l_precomp.h
 *
 * desc:		pre compiler
 *
 * $Archive: /source/code/botlib/l_precomp.h $
 *
 *****************************************************************************/

#ifndef MAX_PATH
	#define MAX_PATH            MAX_QPATH
#endif

#ifndef PATH_SEPERATORSTR
	#if defined(WIN32) | defined(_WIN32) | defined(__NT__) | defined(__WINDOWS__) | defined(__WINDOWS_386__)
		#define PATHSEPERATOR_STR       "\\"
	#else
		#define PATHSEPERATOR_STR       "/"
	#endif
#endif

//set the base folder to load files from
void PC_SetBaseFolder(const char* path);
//load a source file
source_t* LoadSourceFile(const char* filename);
//load a source from memory
source_t* LoadSourceMemory(char* ptr, int length, char* name);
//free the given source
void FreeSource(source_t* source);

//
int PC_LoadSourceHandle(const char* filename);
int PC_FreeSourceHandle(int handle);
int PC_ReadTokenHandle(int handle, q3pc_token_t* pc_token);
int PC_SourceFileAndLine(int handle, char* filename, int* line);
void PC_CheckOpenSourceHandles(void);
