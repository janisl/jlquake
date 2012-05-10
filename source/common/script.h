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
