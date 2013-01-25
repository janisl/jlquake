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

#ifndef __PRECOMPILER_H__
#define __PRECOMPILER_H__

#include "script.h"

//!!!!!!!!!!!!!!! Used by game VMs, do not change !!!!!!!!!!!!!!!!!!!!!
#define MAX_TOKENLENGTH     1024

struct q3pc_token_t {
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[ MAX_TOKENLENGTH ];
};

struct etpc_token_t {
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[ MAX_TOKENLENGTH ];
	int line;
	int linescrossed;
};
//!!!!!!!!!!!!!!! End of stuff used by game VMs !!!!!!!!!!!!!!!!!!!!!

#define DEFINEHASHSIZE      1024

#define DEFINE_FIXED            0x0001

#define INDENT_IF               0x0001
#define INDENT_ELSE             0x0002
#define INDENT_ELIF             0x0004
#define INDENT_IFDEF            0x0008
#define INDENT_IFNDEF           0x0010

//macro definitions
struct define_t {
	char* name;					//define name
	int flags;					//define flags
	int numparms;				//number of define parameters
	token_t* parms;				//define parameters
	token_t* tokens;			//macro tokens (possibly containing parm tokens)
	define_t* next;				//next defined macro in a list
	define_t* hashnext;			//next define in the hash chain
};

//indents
//used for conditional compilation directives:
//#if, #else, #elif, #ifdef, #ifndef
struct indent_t {
	int type;					//indent type
	int skip;					//true if skipping current indent
	script_t* script;			//script the indent was in
	indent_t* next;				//next indent on the indent stack
};

//source file
struct source_t {
	char filename[ MAX_QPATH ];					//file name of the script
	char includepath[ MAX_QPATH ];				//path to include files
	punctuation_t* punctuations;			//punctuations to use
	script_t* scriptstack;					//stack with scripts of the source
	token_t* tokens;						//tokens to read first
	define_t* defines;						//list with macro definitions
	define_t** definehash;					//hash chain with defines
	indent_t* indentstack;					//stack with indents
	int skip;								// > 0 if skipping conditional code
	token_t token;							//last read token
};

//print a source error
void SourceError( source_t* source, const char* str, ... ) id_attribute( ( format( printf, 2, 3 ) ) );
//print a source warning
void SourceWarning( source_t* source, const char* str, ... ) id_attribute( ( format( printf, 2, 3 ) ) );
//add a globals define that will be added to all opened sources
int PC_AddGlobalDefine( const char* string );
//remove all globals defines
void PC_RemoveAllGlobalDefines();
//read a token from the source
bool PC_ReadToken( source_t* source, token_t* token );
//expect a certain token
bool PC_ExpectTokenString( source_t* source, const char* string );
//expect a certain token type
bool PC_ExpectTokenType( source_t* source, int type, int subtype, token_t* token );
//expect a token
bool PC_ExpectAnyToken( source_t* source, token_t* token );
//returns true when the token is available
bool PC_CheckTokenString( source_t* source, const char* string );
//unread the last token read from the script
void PC_UnreadLastToken( source_t* source );
//set the base folder to load files from
void PC_SetBaseFolder( const char* path );
//load a source file
source_t* LoadSourceFile( const char* filename );
//free the given source
void FreeSource( source_t* source );

int PC_LoadSourceHandle( const char* filename );
int PC_FreeSourceHandle( int handle );
int PC_ReadTokenHandleQ3( int handle, q3pc_token_t* pc_token );
int PC_ReadTokenHandleET( int handle, etpc_token_t* pc_token );
void PC_UnreadLastTokenHandle( int handle );
int PC_SourceFileAndLine( int handle, char* filename, int* line );
void PC_CheckOpenSourceHandles();

#endif
