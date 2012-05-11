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

#include "qcommon.h"

//longer punctuations first
static punctuation_t default_punctuations[] =
{
	//binary operators
	{">>=",P_RSHIFT_ASSIGN, NULL},
	{"<<=",P_LSHIFT_ASSIGN, NULL},
	//
	{"...",P_PARMS, NULL},
	//define merge operator
	{"##",P_PRECOMPMERGE, NULL},
	//logic operators
	{"&&",P_LOGIC_AND, NULL},
	{"||",P_LOGIC_OR, NULL},
	{">=",P_LOGIC_GEQ, NULL},
	{"<=",P_LOGIC_LEQ, NULL},
	{"==",P_LOGIC_EQ, NULL},
	{"!=",P_LOGIC_UNEQ, NULL},
	//arithmatic operators
	{"*=",P_MUL_ASSIGN, NULL},
	{"/=",P_DIV_ASSIGN, NULL},
	{"%=",P_MOD_ASSIGN, NULL},
	{"+=",P_ADD_ASSIGN, NULL},
	{"-=",P_SUB_ASSIGN, NULL},
	{"++",P_INC, NULL},
	{"--",P_DEC, NULL},
	//binary operators
	{"&=",P_BIN_AND_ASSIGN, NULL},
	{"|=",P_BIN_OR_ASSIGN, NULL},
	{"^=",P_BIN_XOR_ASSIGN, NULL},
	{">>",P_RSHIFT, NULL},
	{"<<",P_LSHIFT, NULL},
	//reference operators
	{"->",P_POINTERREF, NULL},
	//C++
	{"::",P_CPP1, NULL},
	{".*",P_CPP2, NULL},
	//arithmatic operators
	{"*",P_MUL, NULL},
	{"/",P_DIV, NULL},
	{"%",P_MOD, NULL},
	{"+",P_ADD, NULL},
	{"-",P_SUB, NULL},
	{"=",P_ASSIGN, NULL},
	//binary operators
	{"&",P_BIN_AND, NULL},
	{"|",P_BIN_OR, NULL},
	{"^",P_BIN_XOR, NULL},
	{"~",P_BIN_NOT, NULL},
	//logic operators
	{"!",P_LOGIC_NOT, NULL},
	{">",P_LOGIC_GREATER, NULL},
	{"<",P_LOGIC_LESS, NULL},
	//reference operator
	{".",P_REF, NULL},
	//seperators
	{",",P_COMMA, NULL},
	{";",P_SEMICOLON, NULL},
	//label indication
	{":",P_COLON, NULL},
	//if statement
	{"?",P_QUESTIONMARK, NULL},
	//embracements
	{"(",P_PARENTHESESOPEN, NULL},
	{")",P_PARENTHESESCLOSE, NULL},
	{"{",P_BRACEOPEN, NULL},
	{"}",P_BRACECLOSE, NULL},
	{"[",P_SQBRACKETOPEN, NULL},
	{"]",P_SQBRACKETCLOSE, NULL},
	//
	{"\\",P_BACKSLASH, NULL},
	//precompiler operator
	{"#",P_PRECOMP, NULL},
	{"$",P_DOLLAR, NULL},
	{NULL, 0}
};

static char basefolder[MAX_QPATH];

void PS_SetBaseFolder(const char* path)
{
	String::Sprintf(basefolder, sizeof(basefolder), path);
}

static void PS_CreatePunctuationTable(script_t* script, punctuation_t* punctuations)
{
	//get memory for the table
	if (!script->punctuationtable)
	{
		script->punctuationtable = (punctuation_t**)Mem_Alloc(256 * sizeof(punctuation_t*));
	}
	Com_Memset(script->punctuationtable, 0, 256 * sizeof(punctuation_t*));
	//add the punctuations in the list to the punctuation table
	for (int i = 0; punctuations[i].p; i++)
	{
		punctuation_t* newp = &punctuations[i];
		punctuation_t* lastp = NULL;
		//sort the punctuations in this table entry on length (longer punctuations first)
		punctuation_t* p;
		for (p = script->punctuationtable[(unsigned int)newp->p[0]]; p; p = p->next)
		{
			if (String::Length(p->p) < String::Length(newp->p))
			{
				newp->next = p;
				if (lastp)
				{
					lastp->next = newp;
				}
				else
				{
					script->punctuationtable[(unsigned int)newp->p[0]] = newp;
				}
				break;
			}
			lastp = p;
		}
		if (!p)
		{
			newp->next = NULL;
			if (lastp)
			{
				lastp->next = newp;
			}
			else
			{
				script->punctuationtable[(unsigned int)newp->p[0]] = newp;
			}
		}
	}
}

void SetScriptPunctuations(script_t* script, punctuation_t* p)
{
	if (p)
	{
		PS_CreatePunctuationTable(script, p);
		script->punctuations = p;
	}
	else
	{
		PS_CreatePunctuationTable(script, default_punctuations);
		script->punctuations = default_punctuations;
	}
}

script_t* LoadScriptFile(const char* filename)
{
	char pathname[MAX_QPATH];
	if (String::Length(basefolder))
	{
		String::Sprintf(pathname, sizeof(pathname), "%s/%s", basefolder, filename);
	}
	else
	{
		String::Sprintf(pathname, sizeof(pathname), "%s", filename);
	}
	fileHandle_t fp;
	int length = FS_FOpenFileByMode(pathname, &fp, FS_READ);
	if (!fp)
	{
		return NULL;
	}

	void* buffer = Mem_ClearedAlloc(sizeof(script_t) + length + 1);
	script_t* script = (script_t*)buffer;
	Com_Memset(script, 0, sizeof(script_t));
	String::Cpy(script->filename, filename);
	script->buffer = (char*)buffer + sizeof(script_t);
	script->buffer[length] = 0;
	script->length = length;
	//pointer in script buffer
	script->script_p = script->buffer;
	//pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	//pointer to end of script buffer
	script->end_p = &script->buffer[length];
	//set if there's a token available in script->token
	script->tokenavailable = 0;
	script->line = 1;
	script->lastline = 1;

	SetScriptPunctuations(script, NULL);

	FS_Read(script->buffer, length, fp);
	FS_FCloseFile(fp);

	return script;
}

//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
script_t* LoadScriptMemory(const char* ptr, int length, const char* name)
{
	void* buffer;
	script_t* script;

	buffer = Mem_ClearedAlloc(sizeof(script_t) + length + 1);
	script = (script_t*)buffer;
	Com_Memset(script, 0, sizeof(script_t));
	String::Cpy(script->filename, name);
	script->buffer = (char*)buffer + sizeof(script_t);
	script->buffer[length] = 0;
	script->length = length;
	//pointer in script buffer
	script->script_p = script->buffer;
	//pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	//pointer to end of script buffer
	script->end_p = &script->buffer[length];
	//set if there's a token available in script->token
	script->tokenavailable = 0;
	//
	script->line = 1;
	script->lastline = 1;
	//
	SetScriptPunctuations(script, NULL);
	//
	Com_Memcpy(script->buffer, ptr, length);
	//
	return script;
}	//end of the function LoadScriptMemory

void FreeScript(script_t* script)
{
	if (script->punctuationtable)
	{
		Mem_Free(script->punctuationtable);
	}
	Mem_Free(script);
}

// Reads spaces, tabs, C-like comments etc.
// When a newline character is found the scripts line counter is increased.
int PS_ReadWhiteSpace(script_t* script)
{
	while (1)
	{
		//skip white space
		while (*script->script_p <= ' ')
		{
			if (!*script->script_p)
			{
				return 0;
			}
			if (*script->script_p == '\n')
			{
				script->line++;
			}
			script->script_p++;
		}	//end while
			//skip comments
		if (*script->script_p == '/')
		{
			//comments //
			if (*(script->script_p + 1) == '/')
			{
				script->script_p++;
				do
				{
					script->script_p++;
					if (!*script->script_p)
					{
						return 0;
					}
				}	//end do
				while (*script->script_p != '\n');
				script->line++;
				script->script_p++;
				if (!*script->script_p)
				{
					return 0;
				}
				continue;
			}	//end if
				//comments /* */
			else if (*(script->script_p + 1) == '*')
			{
				script->script_p++;
				do
				{
					script->script_p++;
					if (!*script->script_p)
					{
						return 0;
					}
					if (*script->script_p == '\n')
					{
						script->line++;
					}
				}	//end do
				while (!(*script->script_p == '*' && *(script->script_p + 1) == '/'));
				script->script_p++;
				if (!*script->script_p)
				{
					return 0;
				}
				script->script_p++;
				if (!*script->script_p)
				{
					return 0;
				}
				continue;
			}	//end if
		}	//end if
		break;
	}	//end while
	return 1;
}	//end of the function PS_ReadWhiteSpace
