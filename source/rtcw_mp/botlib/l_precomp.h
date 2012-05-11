/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		l_precomp.h
 *
 * desc:		pre compiler
 *
 *
 *****************************************************************************/

#ifndef _MAX_PATH
	#define MAX_PATH            MAX_QPATH
#endif

#ifndef PATH_SEPERATORSTR
	#if defined(WIN32) | defined(_WIN32) | defined(__NT__) | defined(__WINDOWS__) | defined(__WINDOWS_386__)
		#define PATHSEPERATOR_STR       "\\"
	#else
		#define PATHSEPERATOR_STR       "/"
	#endif
#endif

#define BUILTIN_LINE            1
#define BUILTIN_FILE            2
#define BUILTIN_DATE            3
#define BUILTIN_TIME            4
#define BUILTIN_STDC            5

#define INDENT_IF               0x0001
#define INDENT_ELSE             0x0002
#define INDENT_ELIF             0x0004
#define INDENT_IFDEF            0x0008
#define INDENT_IFNDEF           0x0010

//read a token from the source
int PC_ReadToken(source_t* source, token_t* token);
//expect a certain token
int PC_ExpectTokenString(source_t* source, const char* string);
//expect a certain token type
int PC_ExpectTokenType(source_t* source, int type, int subtype, token_t* token);
//expect a token
int PC_ExpectAnyToken(source_t* source, token_t* token);
//returns true when the token is available
int PC_CheckTokenString(source_t* source, const char* string);
//returns true an reads the token when a token with the given type is available
int PC_CheckTokenType(source_t* source, int type, int subtype, token_t* token);
//skip tokens until the given token string is read
int PC_SkipUntilString(source_t* source, char* string);
//unread the last token read from the script
void PC_UnreadLastToken(source_t* source);
//unread the given token
void PC_UnreadToken(source_t* source, token_t* token);
//add builtin defines
void PC_AddBuiltinDefines(source_t* source);
//set the source include path
void PC_SetIncludePath(source_t* source, char* path);
//set the punction set
void PC_SetPunctuations(source_t* source, punctuation_t* p);
//set the base folder to load files from
void PC_SetBaseFolder(char* path);
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
