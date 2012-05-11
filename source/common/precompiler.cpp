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

token_t* PC_CopyToken(token_t* token)
{
	token_t* t = (token_t*)Mem_Alloc(sizeof(token_t));
	Com_Memcpy(t, token, sizeof(token_t));
	t->next = NULL;
	return t;
}

void PC_FreeToken(token_t* token)
{
	Mem_Free(token);
}

void SourceError(source_t* source, const char* str, ...)
{
	va_list ap;
	char text[1024];
	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	common->Printf(S_COLOR_RED "Error: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

void SourceWarning(source_t* source, const char* str, ...)
{
	va_list ap;
	char text[1024];
	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	common->Printf(S_COLOR_YELLOW "Warning: file %s, line %d: %s\n", source->scriptstack->filename, source->scriptstack->line, text);
}

void PC_PushIndent(source_t* source, int type, int skip)
{
	indent_t* indent = (indent_t*)Mem_Alloc(sizeof(indent_t));
	indent->type = type;
	indent->script = source->scriptstack;
	indent->skip = (skip != 0);
	source->skip += indent->skip;
	indent->next = source->indentstack;
	source->indentstack = indent;
}

void PC_PopIndent(source_t* source, int* type, int* skip)
{
	*type = 0;
	*skip = 0;

	indent_t* indent = source->indentstack;
	if (!indent)
	{
		return;
	}

	//must be an indent from the current script
	if (source->indentstack->script != source->scriptstack)
	{
		return;
	}

	*type = indent->type;
	*skip = indent->skip;
	source->indentstack = source->indentstack->next;
	source->skip -= indent->skip;
	Mem_Free(indent);
}

bool PC_ReadSourceToken(source_t* source, token_t* token)
{
	//if there's no token already available
	while (!source->tokens)
	{
		//if there's a token to read from the script
		if (PS_ReadToken(source->scriptstack, token))
		{
			return true;
		}
		//if at the end of the script
		if (EndOfScript(source->scriptstack))
		{
			//remove all indents of the script
			while (source->indentstack &&
				   source->indentstack->script == source->scriptstack)
			{
				SourceWarning(source, "missing #endif");
				int type, skip;
				PC_PopIndent(source, &type, &skip);
			}
		}
		//if this was the initial script
		if (!source->scriptstack->next)
		{
			return false;
		}
		//remove the script and return to the last one
		script_t* script = source->scriptstack;
		source->scriptstack = source->scriptstack->next;
		FreeScript(script);
	}
	//copy the already available token
	Com_Memcpy(token, source->tokens, sizeof(token_t));
	//free the read token
	token_t* t = source->tokens;
	source->tokens = source->tokens->next;
	PC_FreeToken(t);
	return true;
}

void PC_UnreadSourceToken(source_t* source, token_t* token)
{
	token_t* t = PC_CopyToken(token);
	t->next = source->tokens;
	source->tokens = t;
}

// reads a token from the current line, continues reading on the next
// line only if a backslash '\' is encountered.
bool PC_ReadLine(source_t* source, token_t* token)
{
	bool crossline = false;
	do
	{
		if (!PC_ReadSourceToken(source, token))
		{
			return false;
		}

		if (token->linescrossed > crossline)
		{
			PC_UnreadSourceToken(source, token);
			return false;
		}
		crossline = true;
	}
	while (!String::Cmp(token->string, "\\"));
	return true;
}

define_t* PC_CopyDefine(source_t* source, define_t* define)
{
	define_t* newdefine = (define_t*)Mem_Alloc(sizeof(define_t) + String::Length(define->name) + 1);
	//copy the define name
	newdefine->name = (char*)newdefine + sizeof(define_t);
	String::Cpy(newdefine->name, define->name);
	newdefine->flags = define->flags;
	newdefine->builtin = define->builtin;
	newdefine->numparms = define->numparms;
	//the define is not linked
	newdefine->next = NULL;
	newdefine->hashnext = NULL;
	//copy the define tokens
	newdefine->tokens = NULL;
	for (token_t* lasttoken = NULL, * token = define->tokens; token; token = token->next)
	{
		token_t* newtoken = PC_CopyToken(token);
		newtoken->next = NULL;
		if (lasttoken)
		{
			lasttoken->next = newtoken;
		}
		else
		{
			newdefine->tokens = newtoken;
		}
		lasttoken = newtoken;
	}
	//copy the define parameters
	newdefine->parms = NULL;
	for (token_t* lasttoken = NULL, * token = define->parms; token; token = token->next)
	{
		token_t* newtoken = PC_CopyToken(token);
		newtoken->next = NULL;
		if (lasttoken)
		{
			lasttoken->next = newtoken;
		}
		else
		{
			newdefine->parms = newtoken;
		}
		lasttoken = newtoken;
	}
	return newdefine;
}

void PC_FreeDefine(define_t* define)
{
	token_t* next;
	//free the define parameters
	for (token_t* t = define->parms; t; t = next)
	{
		next = t->next;
		PC_FreeToken(t);
	}
	//free the define tokens
	for (token_t* t = define->tokens; t; t = next)
	{
		next = t->next;
		PC_FreeToken(t);
	}
	//free the define
	Mem_Free(define);
}

int PC_NameHash(const char* name)
{
	int hash = 0;
	for (int i = 0; name[i] != '\0'; i++)
	{
		hash += name[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (DEFINEHASHSIZE - 1);
	return hash;
}

void PC_AddDefineToHash(define_t* define, define_t** definehash)
{
	int hash = PC_NameHash(define->name);
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

define_t* PC_FindHashedDefine(define_t** definehash, const char* name)
{
	int hash = PC_NameHash(name);
	for (define_t* d = definehash[hash]; d; d = d->hashnext)
	{
		if (!String::Cmp(d->name, name))
		{
			return d;
		}
	}
	return NULL;
}

int PC_Directive_undef(source_t* source)
{
	if (source->skip > 0)
	{
		return true;
	}

	token_t token;
	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "undef without name");
		return false;
	}
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name, found %s", token.string);
		return false;
	}

	int hash = PC_NameHash(token.string);
	for (define_t* lastdefine = NULL, * define = source->definehash[hash]; define; define = define->hashnext)
	{
		if (!String::Cmp(define->name, token.string))
		{
			if (define->flags & DEFINE_FIXED)
			{
				SourceWarning(source, "can't undef %s", token.string);
			}
			else
			{
				if (lastdefine)
				{
					lastdefine->hashnext = define->hashnext;
				}
				else
				{
					source->definehash[hash] = define->hashnext;
				}
				PC_FreeDefine(define);
			}
			break;
		}
		lastdefine = define;
	}
	return true;
}
