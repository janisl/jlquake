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

token_t* PC_CopyToken(token_t* token);
void PC_FreeToken(token_t* token);
