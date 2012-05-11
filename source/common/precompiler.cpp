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

#define PATHSEPERATOR_CHAR      '/'

//list with global defines added to every source loaded
static define_t* globaldefines;

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

static define_t* PC_CopyDefine(source_t* source, define_t* define)
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

bool PC_Directive_undef(source_t* source)
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

static bool PC_WhiteSpaceBeforeToken(token_t* token)
{
	return token->endwhitespace_p - token->whitespace_p > 0;
}

static void PC_ClearTokenWhiteSpace(token_t* token)
{
	token->whitespace_p = NULL;
	token->endwhitespace_p = NULL;
	token->linescrossed = 0;
}

int PC_FindDefineParm(define_t* define, const char* name)
{
	int i = 0;
	for (token_t* p = define->parms; p; p = p->next)
	{
		if (!String::Cmp(p->string, name))
		{
			return i;
		}
		i++;
	}
	return -1;
}

bool PC_Directive_define(source_t* source)
{
	if (source->skip > 0)
	{
		return true;
	}

	token_t token;
	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "#define without name");
		return false;
	}
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name after #define, found %s", token.string);
		return false;
	}
	//check if the define already exists
	define_t* define = PC_FindHashedDefine(source->definehash, token.string);
	if (define)
	{
		if (define->flags & DEFINE_FIXED)
		{
			SourceError(source, "can't redefine %s", token.string);
			return false;
		}
		SourceWarning(source, "redefinition of %s", token.string);
		//unread the define name before executing the #undef directive
		PC_UnreadSourceToken(source, &token);
		if (!PC_Directive_undef(source))
		{
			return false;
		}
		//if the define was not removed (define->flags & DEFINE_FIXED)
		define = PC_FindHashedDefine(source->definehash, token.string);
	}
	//allocate define
	define = (define_t*)Mem_Alloc(sizeof(define_t) + String::Length(token.string) + 1);
	Com_Memset(define, 0, sizeof(define_t));
	define->name = (char*)define + sizeof(define_t);
	String::Cpy(define->name, token.string);
	//add the define to the source
	PC_AddDefineToHash(define, source->definehash);
	//if nothing is defined, just return
	if (!PC_ReadLine(source, &token))
	{
		return true;
	}
	//if it is a define with parameters
	if (!PC_WhiteSpaceBeforeToken(&token) && !String::Cmp(token.string, "("))
	{
		//read the define parameters
		token_t* last = NULL;
		if (!PC_ReadLine(source, &token))
		{
			SourceError(source, "expected define parameter");
			return false;
		}
		if (String::Cmp(token.string, ")"))
		{
			PC_UnreadSourceToken(source, &token);
			while (1)
			{
				if (!PC_ReadLine(source, &token))
				{
					SourceError(source, "expected define parameter");
					return false;
				}
				//if it isn't a name
				if (token.type != TT_NAME)
				{
					SourceError(source, "invalid define parameter");
					return false;
				}

				if (PC_FindDefineParm(define, token.string) >= 0)
				{
					SourceError(source, "two the same define parameters");
					return false;
				}
				//add the define parm
				token_t* t = PC_CopyToken(&token);
				PC_ClearTokenWhiteSpace(t);
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					define->parms = t;
				}
				last = t;
				define->numparms++;
				//read next token
				if (!PC_ReadLine(source, &token))
				{
					SourceError(source, "define parameters not terminated");
					return false;
				}

				if (!String::Cmp(token.string, ")"))
				{
					break;
				}
				//then it must be a comma
				if (String::Cmp(token.string, ","))
				{
					SourceError(source, "define not terminated");
					return false;
				}
			}
		}
		if (!PC_ReadLine(source, &token))
		{
			return true;
		}
	}
	//read the defined stuff
	token_t* last = NULL;
	do
	{
		token_t* t = PC_CopyToken(&token);
		if (t->type == TT_NAME && !String::Cmp(t->string, define->name))
		{
			SourceError(source, "recursive define (removed recursion)");
			continue;
		}
		PC_ClearTokenWhiteSpace(t);
		t->next = NULL;
		if (last)
		{
			last->next = t;
		}
		else
		{
			define->tokens = t;
		}
		last = t;
	}
	while (PC_ReadLine(source, &token));

	if (last)
	{
		//check for merge operators at the beginning or end
		if (!String::Cmp(define->tokens->string, "##") ||
			!String::Cmp(last->string, "##"))
		{
			SourceError(source, "define with misplaced ##");
			return false;
		}
	}
	return true;
}

static define_t* PC_DefineFromString(const char* string)
{
	script_t* script = LoadScriptMemory(string, String::Length(string), "*extern");
	//create a new source
	source_t src;
	Com_Memset(&src, 0, sizeof(source_t));
	String::NCpy(src.filename, "*extern", MAX_QPATH);
	src.scriptstack = script;
	src.definehash = (define_t**)Mem_ClearedAlloc(DEFINEHASHSIZE * sizeof(define_t*));
		//create a define from the source
	bool res = PC_Directive_define(&src);
	//free any tokens if left
	for (token_t* t = src.tokens; t; t = src.tokens)
	{
		src.tokens = src.tokens->next;
		PC_FreeToken(t);
	}
	define_t* def = NULL;
	for (int i = 0; i < DEFINEHASHSIZE; i++)
	{
		if (src.definehash[i])
		{
			def = src.definehash[i];
			break;
		}
	}

	Mem_Free(src.definehash);

	FreeScript(script);
	//if the define was created succesfully
	if (res)
	{
		return def;
	}
	//free the define if created
	if (src.defines)
	{
		PC_FreeDefine(def);
	}
	return NULL;
}

// add a globals define that will be added to all opened sources
int PC_AddGlobalDefine(const char* string)
{
	define_t* define = PC_DefineFromString(string);
	if (!define)
	{
		return false;
	}
	define->next = globaldefines;
	globaldefines = define;
	return true;
}

void PC_RemoveAllGlobalDefines()
{
	for (define_t* define = globaldefines; define; define = globaldefines)
	{
		globaldefines = globaldefines->next;
		PC_FreeDefine(define);
	}
}

void PC_AddGlobalDefinesToSource(source_t* source)
{
	for (define_t* define = globaldefines; define; define = define->next)
	{
		define_t* newdefine = PC_CopyDefine(source, define);
		PC_AddDefineToHash(newdefine, source->definehash);
	}
}

void PC_PushScript(source_t* source, script_t* script)
{
	for (script_t* s = source->scriptstack; s; s = s->next)
	{
		if (!String::ICmp(s->filename, script->filename))
		{
			SourceError(source, "%s recursively included", script->filename);
			return;
		}
	}
	//push the script on the script stack
	script->next = source->scriptstack;
	source->scriptstack = script;
}

void PC_ConvertPath(char* path)
{
	char* ptr;

	//remove double path seperators
	for (ptr = path; *ptr; )
	{
		if ((*ptr == '\\' || *ptr == '/') &&
			(*(ptr + 1) == '\\' || *(ptr + 1) == '/'))
		{
			memmove(ptr, ptr + 1, String::Length(ptr));
		}	//end if
		else
		{
			ptr++;
		}	//end else
	}	//end while
		//set OS dependent path seperators
	for (ptr = path; *ptr; )
	{
		if (*ptr == '/' || *ptr == '\\')
		{
			*ptr = PATHSEPERATOR_CHAR;
		}
		ptr++;
	}
}

bool PC_Directive_include(source_t* source)
{
	if (source->skip > 0)
	{
		return true;
	}

	token_t token;
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "#include without file name");
		return false;
	}
	if (token.linescrossed > 0)
	{
		SourceError(source, "#include without file name");
		return false;
	}
	char path[MAX_QPATH];
	script_t* script;
	if (token.type == TT_STRING)
	{
		StripDoubleQuotes(token.string);
		PC_ConvertPath(token.string);
		script = LoadScriptFile(token.string);
		if (!script)
		{
			String::Cpy(path, source->includepath);
			String::Cat(path, sizeof(path), token.string);
			script = LoadScriptFile(path);
		}
	}
	else if (token.type == TT_PUNCTUATION && *token.string == '<')
	{
		String::Cpy(path, source->includepath);
		while (PC_ReadSourceToken(source, &token))
		{
			if (token.linescrossed > 0)
			{
				PC_UnreadSourceToken(source, &token);
				break;
			}
			if (token.type == TT_PUNCTUATION && *token.string == '>')
			{
				break;
			}
			String::Cat(path, sizeof(path), token.string);
		}
		if (*token.string != '>')
		{
			SourceWarning(source, "#include missing trailing >");
		}
		if (!String::Length(path))
		{
			SourceError(source, "#include without file name between < >");
			return false;
		}
		PC_ConvertPath(path);
		script = LoadScriptFile(path);
	}
	else
	{
		SourceError(source, "#include without file name");
		return false;
	}
	if (!script)
	{
		SourceError(source, "file %s not found", path);
		return false;
	}
	PC_PushScript(source, script);
	return true;
}
