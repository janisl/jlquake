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
//

/*****************************************************************************
 * name:		l_precomp.c
 *
 * desc:		pre compiler
 *
 * $Archive: /MissionPack/code/botlib/l_precomp.c $
 *
 *****************************************************************************/

//Notes:			fix: PC_StringizeTokens

#include <time.h>
#include "../common/qcommon.h"
#include "l_precomp.h"

//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpectTokenString(source_t* source, const char* string)
{
	token_t token;

	if (!PC_ReadToken(source, &token))
	{
		SourceError(source, "couldn't find expected %s", string);
		return false;
	}	//end if

	if (String::Cmp(token.string, string))
	{
		SourceError(source, "expected %s, found %s", string, token.string);
		return false;
	}	//end if
	return true;
}	//end of the function PC_ExpectTokenString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpectTokenType(source_t* source, int type, int subtype, token_t* token)
{
	char str[MAX_TOKEN];

	if (!PC_ReadToken(source, token))
	{
		SourceError(source, "couldn't read expected token");
		return false;
	}	//end if

	if (token->type != type)
	{
		String::Cpy(str, "");
		if (type == TT_STRING)
		{
			String::Cpy(str, "string");
		}
		if (type == TT_LITERAL)
		{
			String::Cpy(str, "literal");
		}
		if (type == TT_NUMBER)
		{
			String::Cpy(str, "number");
		}
		if (type == TT_NAME)
		{
			String::Cpy(str, "name");
		}
		if (type == TT_PUNCTUATION)
		{
			String::Cpy(str, "punctuation");
		}
		SourceError(source, "expected a %s, found %s", str, token->string);
		return false;
	}	//end if
	if (token->type == TT_NUMBER)
	{
		if ((token->subtype & subtype) != subtype)
		{
			if (subtype & TT_DECIMAL)
			{
				String::Cpy(str, "decimal");
			}
			if (subtype & TT_HEX)
			{
				String::Cpy(str, "hex");
			}
			if (subtype & TT_OCTAL)
			{
				String::Cpy(str, "octal");
			}
			if (subtype & TT_BINARY)
			{
				String::Cpy(str, "binary");
			}
			if (subtype & TT_LONG)
			{
				String::Cat(str, sizeof(str), " long");
			}
			if (subtype & TT_UNSIGNED)
			{
				String::Cat(str, sizeof(str), " unsigned");
			}
			if (subtype & TT_FLOAT)
			{
				String::Cat(str, sizeof(str), " float");
			}
			if (subtype & TT_INTEGER)
			{
				String::Cat(str, sizeof(str), " integer");
			}
			SourceError(source, "expected %s, found %s", str, token->string);
			return false;
		}	//end if
	}	//end if
	else if (token->type == TT_PUNCTUATION)
	{
		if (token->subtype != subtype)
		{
			SourceError(source, "found %s", token->string);
			return false;
		}	//end if
	}	//end else if
	return true;
}	//end of the function PC_ExpectTokenType
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpectAnyToken(source_t* source, token_t* token)
{
	if (!PC_ReadToken(source, token))
	{
		SourceError(source, "couldn't read expected token");
		return false;
	}	//end if
	else
	{
		return true;
	}	//end else
}	//end of the function PC_ExpectAnyToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_CheckTokenString(source_t* source, const char* string)
{
	token_t tok;

	if (!PC_ReadToken(source, &tok))
	{
		return false;
	}
	//if the token is available
	if (!String::Cmp(tok.string, string))
	{
		return true;
	}
	//
	PC_UnreadSourceToken(source, &tok);
	return false;
}	//end of the function PC_CheckTokenString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_CheckTokenType(source_t* source, int type, int subtype, token_t* token)
{
	token_t tok;

	if (!PC_ReadToken(source, &tok))
	{
		return false;
	}
	//if the type matches
	if (tok.type == type &&
		(tok.subtype & subtype) == subtype)
	{
		Com_Memcpy(token, &tok, sizeof(token_t));
		return true;
	}	//end if
		//
	PC_UnreadSourceToken(source, &tok);
	return false;
}	//end of the function PC_CheckTokenType
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_SkipUntilString(source_t* source, char* string)
{
	token_t token;

	while (PC_ReadToken(source, &token))
	{
		if (!String::Cmp(token.string, string))
		{
			return true;
		}
	}	//end while
	return false;
}	//end of the function PC_SkipUntilString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_UnreadLastToken(source_t* source)
{
	PC_UnreadSourceToken(source, &source->token);
}	//end of the function PC_UnreadLastToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_UnreadToken(source_t* source, token_t* token)
{
	PC_UnreadSourceToken(source, token);
}	//end of the function PC_UnreadToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_SetIncludePath(source_t* source, char* path)
{
	String::NCpy(source->includepath, path, MAX_QPATH);
	//add trailing path seperator
	if (source->includepath[String::Length(source->includepath) - 1] != '\\' &&
		source->includepath[String::Length(source->includepath) - 1] != '/')
	{
		String::Cat(source->includepath, MAX_QPATH, PATHSEPERATOR_STR);
	}	//end if
}	//end of the function PC_SetIncludePath
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_SetPunctuations(source_t* source, punctuation_t* p)
{
	source->punctuations = p;
}	//end of the function PC_SetPunctuations
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
source_t* LoadSourceFile(const char* filename)
{
	source_t* source;
	script_t* script;

	script = LoadScriptFile(filename);
	if (!script)
	{
		return NULL;
	}

	script->next = NULL;

	source = (source_t*)Mem_Alloc(sizeof(source_t));
	Com_Memset(source, 0, sizeof(source_t));

	String::NCpy(source->filename, filename, MAX_QPATH);
	source->scriptstack = script;
	source->tokens = NULL;
	source->defines = NULL;
	source->indentstack = NULL;
	source->skip = 0;

	source->definehash = (define_t**)Mem_ClearedAlloc(DEFINEHASHSIZE * sizeof(define_t*));
	PC_AddGlobalDefinesToSource(source);
	return source;
}	//end of the function LoadSourceFile
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
source_t* LoadSourceMemory(char* ptr, int length, char* name)
{
	source_t* source;
	script_t* script;

	script = LoadScriptMemory(ptr, length, name);
	if (!script)
	{
		return NULL;
	}
	script->next = NULL;

	source = (source_t*)Mem_Alloc(sizeof(source_t));
	Com_Memset(source, 0, sizeof(source_t));

	String::NCpy(source->filename, name, MAX_QPATH);
	source->scriptstack = script;
	source->tokens = NULL;
	source->defines = NULL;
	source->indentstack = NULL;
	source->skip = 0;

	source->definehash = (define_t**)Mem_ClearedAlloc(DEFINEHASHSIZE * sizeof(define_t*));
	PC_AddGlobalDefinesToSource(source);
	return source;
}	//end of the function LoadSourceMemory
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void FreeSource(source_t* source)
{
	script_t* script;
	token_t* token;
	define_t* define;
	indent_t* indent;
	int i;

	//free all the scripts
	while (source->scriptstack)
	{
		script = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	}	//end for
		//free all the tokens
	while (source->tokens)
	{
		token = source->tokens;
		source->tokens = source->tokens->next;
		PC_FreeToken(token);
	}	//end for
	for (i = 0; i < DEFINEHASHSIZE; i++)
	{
		while (source->definehash[i])
		{
			define = source->definehash[i];
			source->definehash[i] = source->definehash[i]->hashnext;
			PC_FreeDefine(define);
		}	//end while
	}	//end for
		//free all indents
	while (source->indentstack)
	{
		indent = source->indentstack;
		source->indentstack = source->indentstack->next;
		Mem_Free(indent);
	}	//end for
	//
	if (source->definehash)
	{
		Mem_Free(source->definehash);
	}
		//free the source itself
	Mem_Free(source);
}	//end of the function FreeSource
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================

#define MAX_SOURCEFILES     64

source_t* sourceFiles[MAX_SOURCEFILES];

int PC_LoadSourceHandle(const char* filename)
{
	source_t* source;
	int i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (!sourceFiles[i])
		{
			break;
		}
	}	//end for
	if (i >= MAX_SOURCEFILES)
	{
		return 0;
	}
	PS_SetBaseFolder("");
	source = LoadSourceFile(filename);
	if (!source)
	{
		return 0;
	}
	sourceFiles[i] = source;
	return i;
}	//end of the function PC_LoadSourceHandle
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
int PC_FreeSourceHandle(int handle)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return false;
	}
	if (!sourceFiles[handle])
	{
		return false;
	}

	FreeSource(sourceFiles[handle]);
	sourceFiles[handle] = NULL;
	return true;
}	//end of the function PC_FreeSourceHandle
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
int PC_ReadTokenHandle(int handle, q3pc_token_t* pc_token)
{
	token_t token;
	int ret;

	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return 0;
	}
	if (!sourceFiles[handle])
	{
		return 0;
	}

	ret = PC_ReadToken(sourceFiles[handle], &token);
	String::Cpy(pc_token->string, token.string);
	pc_token->type = token.type;
	pc_token->subtype = token.subtype;
	pc_token->intvalue = token.intvalue;
	pc_token->floatvalue = token.floatvalue;
	if (pc_token->type == TT_STRING)
	{
		StripDoubleQuotes(pc_token->string);
	}
	return ret;
}	//end of the function PC_ReadTokenHandle
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
int PC_SourceFileAndLine(int handle, char* filename, int* line)
{
	if (handle < 1 || handle >= MAX_SOURCEFILES)
	{
		return false;
	}
	if (!sourceFiles[handle])
	{
		return false;
	}

	String::Cpy(filename, sourceFiles[handle]->filename);
	if (sourceFiles[handle]->scriptstack)
	{
		*line = sourceFiles[handle]->scriptstack->line;
	}
	else
	{
		*line = 0;
	}
	return true;
}	//end of the function PC_SourceFileAndLine
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
void PC_SetBaseFolder(const char* path)
{
	PS_SetBaseFolder(path);
}	//end of the function PC_SetBaseFolder
//============================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//============================================================================
void PC_CheckOpenSourceHandles(void)
{
	int i;

	for (i = 1; i < MAX_SOURCEFILES; i++)
	{
		if (sourceFiles[i])
		{
			common->Printf(S_COLOR_RED "Error: file %s still open in precompiler\n", sourceFiles[i]->scriptstack->filename);
		}	//end if
	}	//end for
}	//end of the function PC_CheckOpenSourceHandles
