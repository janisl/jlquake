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

//!!!!!!!!!!!!!!! Used by game VMs, do not change !!!!!!!!!!!!!!!!!!!!!
#define MAX_TOKENLENGTH     1024

struct q3pc_token_t
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
};

struct etpc_token_t
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
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
struct define_t
{
	char* name;					//define name
	int flags;					//define flags
	int builtin;				// > 0 if builtin define
	int numparms;				//number of define parameters
	token_t* parms;				//define parameters
	token_t* tokens;			//macro tokens (possibly containing parm tokens)
	define_t* next;				//next defined macro in a list
	define_t* hashnext;			//next define in the hash chain
};

//indents
//used for conditional compilation directives:
//#if, #else, #elif, #ifdef, #ifndef
struct indent_t
{
	int type;					//indent type
	int skip;					//true if skipping current indent
	script_t* script;			//script the indent was in
	indent_t* next;				//next indent on the indent stack
};

//source file
struct source_t
{
	char filename[MAX_QPATH];				//file name of the script
	char includepath[MAX_QPATH];			//path to include files
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
void SourceError(source_t* source, const char* str, ...) id_attribute((format(printf, 2, 3)));
//print a source warning
void SourceWarning(source_t* source, const char* str, ...) id_attribute((format(printf, 2, 3)));
//read a token only if on the same line, lines are concatenated with a slash
bool PC_ReadLine(source_t* source, token_t* token);
//add a globals define that will be added to all opened sources
int PC_AddGlobalDefine(const char* string);
//remove all globals defines
void PC_RemoveAllGlobalDefines();

token_t* PC_CopyToken(token_t* token);
void PC_FreeToken(token_t* token);
void PC_PushIndent(source_t* source, int type, int skip);
void PC_PopIndent(source_t* source, int* type, int* skip);
bool PC_ReadSourceToken(source_t* source, token_t* token);
void PC_UnreadSourceToken(source_t* source, token_t* token);
void PC_FreeDefine(define_t* define);
int PC_NameHash(const char* name);
void PC_AddDefineToHash(define_t* define, define_t** definehash);
define_t* PC_FindHashedDefine(define_t** definehash, const char* name);
bool PC_Directive_undef(source_t* source);
int PC_FindDefineParm(define_t* define, const char* name);
bool PC_Directive_define(source_t* source);
void PC_AddGlobalDefinesToSource(source_t* source);
void PC_PushScript(source_t* source, script_t* script);
void PC_ConvertPath(char* path);
bool PC_Directive_include(source_t* source);
bool PC_Directive_ifdef(source_t* source);
bool PC_Directive_ifndef(source_t* source);
bool PC_Directive_else(source_t* source);
bool PC_Directive_endif(source_t* source);
