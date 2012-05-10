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

//punctuation sub type
#define P_RSHIFT_ASSIGN             1
#define P_LSHIFT_ASSIGN             2
#define P_PARMS                     3
#define P_PRECOMPMERGE              4

#define P_LOGIC_AND                 5
#define P_LOGIC_OR                  6
#define P_LOGIC_GEQ                 7
#define P_LOGIC_LEQ                 8
#define P_LOGIC_EQ                  9
#define P_LOGIC_UNEQ                10

#define P_MUL_ASSIGN                11
#define P_DIV_ASSIGN                12
#define P_MOD_ASSIGN                13
#define P_ADD_ASSIGN                14
#define P_SUB_ASSIGN                15
#define P_INC                       16
#define P_DEC                       17

#define P_BIN_AND_ASSIGN            18
#define P_BIN_OR_ASSIGN             19
#define P_BIN_XOR_ASSIGN            20
#define P_RSHIFT                    21
#define P_LSHIFT                    22

#define P_POINTERREF                23
#define P_CPP1                      24
#define P_CPP2                      25
#define P_MUL                       26
#define P_DIV                       27
#define P_MOD                       28
#define P_ADD                       29
#define P_SUB                       30
#define P_ASSIGN                    31

#define P_BIN_AND                   32
#define P_BIN_OR                    33
#define P_BIN_XOR                   34
#define P_BIN_NOT                   35

#define P_LOGIC_NOT                 36
#define P_LOGIC_GREATER             37
#define P_LOGIC_LESS                38

#define P_REF                       39
#define P_COMMA                     40
#define P_SEMICOLON                 41
#define P_COLON                     42
#define P_QUESTIONMARK              43

#define P_PARENTHESESOPEN           44
#define P_PARENTHESESCLOSE          45
#define P_BRACEOPEN                 46
#define P_BRACECLOSE                47
#define P_SQBRACKETOPEN             48
#define P_SQBRACKETCLOSE            49
#define P_BACKSLASH                 50

#define P_PRECOMP                   51
#define P_DOLLAR                    52

//maximum token length
#define MAX_TOKEN                   1024

struct punctuation_t
{
	const char* p;				//punctuation character(s)
	int n;						//punctuation indication
	punctuation_t* next;		//next punctuation
};

struct token_t
{
	char string[MAX_TOKEN];		//available token
	int type;					//last read token type
	int subtype;				//last read token sub type
	unsigned int intvalue;		//integer value
	long double floatvalue;		//floating point value
	char* whitespace_p;			//start of white space before token
	char* endwhitespace_p;		//start of white space before token
	int line;					//line the token was on
	int linescrossed;			//lines crossed in white space
	token_t* next;				//next token in chain
};

struct script_t
{
	char filename[MAX_QPATH];		//file name of the script
	char* buffer;					//buffer containing the script
	char* script_p;					//current pointer in the script
	char* end_p;					//pointer to the end of the script
	char* lastscript_p;				//script pointer before reading token
	char* whitespace_p;				//begin of the white space
	char* endwhitespace_p;			//end of the white space
	int length;						//length of the script in bytes
	int line;						//current line in script
	int lastline;					//line before reading token
	int tokenavailable;				//set by UnreadLastToken
	int flags;						//several script flags
	punctuation_t* punctuations;	//the punctuations used in the script
	punctuation_t** punctuationtable;
	token_t token;					//available token
	script_t* next;					//next script in a chain
};

//set an array with punctuations, NULL restores default C/C++ set
void SetScriptPunctuations(script_t* script, punctuation_t* p);
