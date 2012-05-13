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

#define MAX_DEFINEPARMS         128

struct operator_t
{
	int oper;
	int priority;
	int parentheses;
	operator_t* prev, * next;
};

struct value_t
{
	signed int intvalue;
	double floatvalue;
	int parentheses;
	value_t* prev, * next;
};

//directive name with parse function
struct directive_t
{
	const char* name;
	bool (*func)(source_t* source);
};

//list with global defines added to every source loaded
static define_t* globaldefines;

static token_t* PC_CopyToken(token_t* token)
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

static void PC_PushIndent(source_t* source, int type, int skip)
{
	indent_t* indent = (indent_t*)Mem_Alloc(sizeof(indent_t));
	indent->type = type;
	indent->script = source->scriptstack;
	indent->skip = (skip != 0);
	source->skip += indent->skip;
	indent->next = source->indentstack;
	source->indentstack = indent;
}

static void PC_PopIndent(source_t* source, int* type, int* skip)
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

static bool PC_ReadSourceToken(source_t* source, token_t* token)
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
static bool PC_ReadLine(source_t* source, token_t* token)
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

static int PC_NameHash(const char* name)
{
	int hash = 0;
	for (int i = 0; name[i] != '\0'; i++)
	{
		hash += name[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (DEFINEHASHSIZE - 1);
	return hash;
}

static void PC_AddDefineToHash(define_t* define, define_t** definehash)
{
	int hash = PC_NameHash(define->name);
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

static define_t* PC_FindHashedDefine(define_t** definehash, const char* name)
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

static bool PC_Directive_undef(source_t* source)
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

static int PC_FindDefineParm(define_t* define, const char* name)
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

static bool PC_Directive_define(source_t* source)
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

static void PC_PushScript(source_t* source, script_t* script)
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

static void PC_ConvertPath(char* path)
{
	char* ptr;

	//remove double path seperators
	for (ptr = path; *ptr; )
	{
		if ((*ptr == '\\' || *ptr == '/') &&
			(*(ptr + 1) == '\\' || *(ptr + 1) == '/'))
		{
			memmove(ptr, ptr + 1, String::Length(ptr));
		}
		else
		{
			ptr++;
		}	//end else
	}
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

static bool PC_Directive_include(source_t* source)
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

static int PC_Directive_if_def(source_t* source, int type)
{
	token_t token;
	define_t* d;
	int skip;

	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "#ifdef without name");
		return false;
	}
	if (token.type != TT_NAME)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "expected name after #ifdef, found %s", token.string);
		return false;
	}
	d = PC_FindHashedDefine(source->definehash, token.string);
	skip = (type == INDENT_IFDEF) == (d == NULL);
	PC_PushIndent(source, type, skip);
	return true;
}

static bool PC_Directive_ifdef(source_t* source)
{
	return PC_Directive_if_def(source, INDENT_IFDEF);
}

static bool PC_Directive_ifndef(source_t* source)
{
	return PC_Directive_if_def(source, INDENT_IFNDEF);
}

static bool PC_Directive_else(source_t* source)
{
	int type, skip;
	PC_PopIndent(source, &type, &skip);
	if (!type)
	{
		SourceError(source, "misplaced #else");
		return false;
	}
	if (type == INDENT_ELSE)
	{
		SourceError(source, "#else after #else");
		return false;
	}
	PC_PushIndent(source, INDENT_ELSE, !skip);
	return true;
}

static bool PC_Directive_endif(source_t* source)
{
	int type, skip;
	PC_PopIndent(source, &type, &skip);
	if (!type)
	{
		SourceError(source, "misplaced #endif");
		return false;
	}
	return true;
}

static bool PC_ReadDefineParms(source_t* source, define_t* define, token_t** parms, int maxparms)
{
	token_t token;
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "define %s missing parms", define->name);
		return false;
	}

	if (define->numparms > maxparms)
	{
		SourceError(source, "define with more than %d parameters", maxparms);
		return false;
	}

	for (int i = 0; i < define->numparms; i++)
	{
		parms[i] = NULL;
	}
	//if no leading "("
	if (String::Cmp(token.string, "("))
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "define %s missing parms", define->name);
		return false;
	}
	//read the define parameters
	int numparms = 0;
	int indent = 1;
	for (bool done = false; !done; )
	{
		if (numparms >= maxparms)
		{
			SourceError(source, "define %s with too many parms", define->name);
			return false;
		}
		if (numparms >= define->numparms)
		{
			SourceWarning(source, "define %s has too many parms", define->name);
			return false;
		}
		parms[numparms] = NULL;
		int lastcomma = 1;
		token_t* last = NULL;
		while (!done)
		{
			if (!PC_ReadSourceToken(source, &token))
			{
				SourceError(source, "define %s incomplete", define->name);
				return false;
			}

			if (!String::Cmp(token.string, ","))
			{
				if (indent <= 1)
				{
					if (lastcomma)
					{
						SourceWarning(source, "too many comma's");
					}
					lastcomma = 1;
					break;
				}
			}
			lastcomma = 0;

			if (!String::Cmp(token.string, "("))
			{
				indent++;
			}
			else if (!String::Cmp(token.string, ")"))
			{
				if (--indent <= 0)
				{
					if (!parms[define->numparms - 1])
					{
						SourceWarning(source, "too few define parms");
					}
					done = true;
					break;
				}
			}

			if (numparms < define->numparms)
			{
				token_t* t = PC_CopyToken(&token);
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					parms[numparms] = t;
				}
				last = t;
			}
		}
		numparms++;
	}
	return true;
}

static bool PC_StringizeTokens(token_t* tokens, token_t* token)
{
	token->type = TT_STRING;
	token->whitespace_p = NULL;
	token->endwhitespace_p = NULL;
	token->string[0] = '\0';
	String::Cat(token->string, MAX_TOKEN, "\"");
	for (token_t* t = tokens; t; t = t->next)
	{
		String::Cat(token->string, MAX_TOKEN, t->string);
	}
	String::Cat(token->string, MAX_TOKEN, "\"");
	return true;
}

static bool PC_MergeTokens(token_t* t1, token_t* t2)
{
	//merging of a name with a name or number
	if (t1->type == TT_NAME && (t2->type == TT_NAME || t2->type == TT_NUMBER))
	{
		String::Cat(t1->string, MAX_TOKEN, t2->string);
		return true;
	}
	//merging of two strings
	if (t1->type == TT_STRING && t2->type == TT_STRING)
	{
		//remove trailing double quote
		t1->string[String::Length(t1->string) - 1] = '\0';
		//concat without leading double quote
		String::Cat(t1->string, MAX_TOKEN, &t2->string[1]);
		return true;
	}
	//FIXME: merging of two number of the same sub type
	return false;
}

static bool PC_ExpandDefine(source_t* source, token_t* deftoken, define_t* define,
	token_t** firsttoken, token_t** lasttoken)
{
	//if the define has parameters
	token_t* parms[MAX_DEFINEPARMS];
	if (define->numparms)
	{
		if (!PC_ReadDefineParms(source, define, parms, MAX_DEFINEPARMS))
		{
			return false;
		}
	}
	//empty list at first
	token_t* first = NULL;
	token_t* last = NULL;
	//create a list with tokens of the expanded define
	for (token_t* dt = define->tokens; dt; dt = dt->next)
	{
		int parmnum = -1;
		//if the token is a name, it could be a define parameter
		if (dt->type == TT_NAME)
		{
			parmnum = PC_FindDefineParm(define, dt->string);
		}
		//if it is a define parameter
		if (parmnum >= 0)
		{
			for (token_t* pt = parms[parmnum]; pt; pt = pt->next)
			{
				token_t* t = PC_CopyToken(pt);
				//add the token to the list
				t->next = NULL;
				if (last)
				{
					last->next = t;
				}
				else
				{
					first = t;
				}
				last = t;
			}
		}
		else
		{
			token_t* t;
			//if stringizing operator
			if (dt->string[0] == '#' && dt->string[1] == '\0')
			{
				//the stringizing operator must be followed by a define parameter
				if (dt->next)
				{
					parmnum = PC_FindDefineParm(define, dt->next->string);
				}
				else
				{
					parmnum = -1;
				}
				//
				if (parmnum >= 0)
				{
					//step over the stringizing operator
					dt = dt->next;
					//stringize the define parameter tokens
					token_t token;
					if (!PC_StringizeTokens(parms[parmnum], &token))
					{
						SourceError(source, "can't stringize tokens");
						return false;
					}
					t = PC_CopyToken(&token);
				}
				else
				{
					SourceWarning(source, "stringizing operator without define parameter");
					continue;
				}
			}
			else
			{
				t = PC_CopyToken(dt);
			}
			//add the token to the list
			t->next = NULL;
			if (last)
			{
				last->next = t;
			}
			else
			{
				first = t;
			}
			last = t;
		}
	}
	//check for the merging operator
	for (token_t* t = first; t; )
	{
		if (t->next)
		{
			//if the merging operator
			if (t->next->string[0] == '#' && t->next->string[1] == '#')
			{
				token_t* t1 = t;
				token_t* t2 = t->next->next;
				if (t2)
				{
					if (!PC_MergeTokens(t1, t2))
					{
						SourceError(source, "can't merge %s with %s", t1->string, t2->string);
						return false;
					}
					PC_FreeToken(t1->next);
					t1->next = t2->next;
					if (t2 == last)
					{
						last = t1;
					}
					PC_FreeToken(t2);
					continue;
				}
			}
		}
		t = t->next;
	}
	//store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;
	//free all the parameter tokens
	for (int i = 0; i < define->numparms; i++)
	{
		token_t* nextpt;
		for (token_t* pt = parms[i]; pt; pt = nextpt)
		{
			nextpt = pt->next;
			PC_FreeToken(pt);
		}
	}

	return true;
}

static bool PC_ExpandDefineIntoSource(source_t* source, token_t* deftoken, define_t* define)
{
	token_t* firsttoken, * lasttoken;
	if (!PC_ExpandDefine(source, deftoken, define, &firsttoken, &lasttoken))
	{
		return false;
	}

	if (firsttoken && lasttoken)
	{
		lasttoken->next = source->tokens;
		source->tokens = firsttoken;
		return true;
	}
	return false;
}

static int PC_OperatorPriority(int op)
{
	switch (op)
	{
	case P_MUL: return 15;
	case P_DIV: return 15;
	case P_MOD: return 15;
	case P_ADD: return 14;
	case P_SUB: return 14;

	case P_LOGIC_AND: return 7;
	case P_LOGIC_OR: return 6;
	case P_LOGIC_GEQ: return 12;
	case P_LOGIC_LEQ: return 12;
	case P_LOGIC_EQ: return 11;
	case P_LOGIC_UNEQ: return 11;

	case P_LOGIC_NOT: return 16;
	case P_LOGIC_GREATER: return 12;
	case P_LOGIC_LESS: return 12;

	case P_RSHIFT: return 13;
	case P_LSHIFT: return 13;

	case P_BIN_AND: return 10;
	case P_BIN_OR: return 8;
	case P_BIN_XOR: return 9;
	case P_BIN_NOT: return 16;

	case P_COLON: return 5;
	case P_QUESTIONMARK: return 5;
	}
	return false;
}

#define MAX_VALUES      64
#define MAX_OPERATORS   64
#define AllocValue(val)									\
	if (numvalues >= MAX_VALUES) {						\
		SourceError(source, "out of value space\n");	\
		error = 1;										\
		break;											\
	}													\
	else {												\
		val = &value_heap[numvalues++]; }
#define FreeValue(val)
//
#define AllocOperator(op)								\
	if (numoperators >= MAX_OPERATORS) {				\
		SourceError(source, "out of operator space\n");	\
		error = 1;										\
		break;											\
	}													\
	else {												\
		op = &operator_heap[numoperators++]; }
#define FreeOperator(op)

static bool PC_EvaluateTokens(source_t* source, token_t* tokens, int* intvalue,
	double* floatvalue, bool integer)
{
	operator_t* o, * firstoperator, * lastoperator;
	value_t* v, * firstvalue, * lastvalue, * v1, * v2;
	token_t* t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastwasvalue = 0;
	int negativevalue = 0;
	int questmarkintvalue = 0;
	double questmarkfloatvalue = 0;
	int gotquestmarkvalue = false;
	int lastoperatortype = 0;
	//
	operator_t operator_heap[MAX_OPERATORS];
	int numoperators = 0;
	value_t value_heap[MAX_VALUES];
	int numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	for (t = tokens; t; t = t->next)
	{
		switch (t->type)
		{
		case TT_NAME:
		{
			if (lastwasvalue || negativevalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}	
			if (String::Cmp(t->string, "defined"))
			{
				SourceError(source, "undefined name %s in #if/#elif", t->string);
				error = 1;
				break;
			}	
			t = t->next;
			if (!String::Cmp(t->string, "("))
			{
				brace = true;
				t = t->next;
			}	
			if (!t || t->type != TT_NAME)
			{
				SourceError(source, "defined without name in #if/#elif");
				error = 1;
				break;
			}	
			AllocValue(v);
			if (PC_FindHashedDefine(source->definehash, t->string))
			{
				v->intvalue = 1;
				v->floatvalue = 1;
			}	
			else
			{
				v->intvalue = 0;
				v->floatvalue = 0;
			}
			v->parentheses = parentheses;
			v->next = NULL;
			v->prev = lastvalue;
			if (lastvalue)
			{
				lastvalue->next = v;
			}
			else
			{
				firstvalue = v;
			}
			lastvalue = v;
			if (brace)
			{
				t = t->next;
				if (!t || String::Cmp(t->string, ")"))
				{
					SourceError(source, "defined without ) in #if/#elif");
					error = 1;
					break;
				}	
			}	
			brace = false;
			// defined() creates a value
			lastwasvalue = 1;
			break;
		}
		case TT_NUMBER:
		{
			if (lastwasvalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}	
			AllocValue(v);
			if (negativevalue)
			{
				v->intvalue = -(signed int)t->intvalue;
				v->floatvalue = -t->floatvalue;
			}	
			else
			{
				v->intvalue = t->intvalue;
				v->floatvalue = t->floatvalue;
			}
			v->parentheses = parentheses;
			v->next = NULL;
			v->prev = lastvalue;
			if (lastvalue)
			{
				lastvalue->next = v;
			}
			else
			{
				firstvalue = v;
			}
			lastvalue = v;
			//last token was a value
			lastwasvalue = 1;
			//
			negativevalue = 0;
			break;
		}
		case TT_PUNCTUATION:
		{
			if (negativevalue)
			{
				SourceError(source, "misplaced minus sign in #if/#elif");
				error = 1;
				break;
			}	
			if (t->subtype == P_PARENTHESESOPEN)
			{
				parentheses++;
				break;
			}	
			else if (t->subtype == P_PARENTHESESCLOSE)
			{
				parentheses--;
				if (parentheses < 0)
				{
					SourceError(source, "too many ) in #if/#elsif");
					error = 1;
				}	
				break;
			}
			//check for invalid operators on floating point values
			if (!integer)
			{
				if (t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
					t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
					t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
					t->subtype == P_BIN_XOR)
				{
					SourceError(source, "illigal operator %s on floating point operands\n", t->string);
					error = 1;
					break;
				}	
			}	
			switch (t->subtype)
			{
			case P_LOGIC_NOT:
			case P_BIN_NOT:
			{
				if (lastwasvalue)
				{
					SourceError(source, "! or ~ after value in #if/#elif");
					error = 1;
					break;
				}		
				break;
			}	
			case P_INC:
			case P_DEC:
			{
				SourceError(source, "++ or -- used in #if/#elif");
				break;
			}	
			case P_SUB:
			{
				if (!lastwasvalue)
				{
					negativevalue = 1;
					break;
				}		
			}	

			case P_MUL:
			case P_DIV:
			case P_MOD:
			case P_ADD:

			case P_LOGIC_AND:
			case P_LOGIC_OR:
			case P_LOGIC_GEQ:
			case P_LOGIC_LEQ:
			case P_LOGIC_EQ:
			case P_LOGIC_UNEQ:

			case P_LOGIC_GREATER:
			case P_LOGIC_LESS:

			case P_RSHIFT:
			case P_LSHIFT:

			case P_BIN_AND:
			case P_BIN_OR:
			case P_BIN_XOR:

			case P_COLON:
			case P_QUESTIONMARK:
			{
				if (!lastwasvalue)
				{
					SourceError(source, "operator %s after operator in #if/#elif", t->string);
					error = 1;
					break;
				}		
				break;
			}	
			default:
			{
				SourceError(source, "invalid operator %s in #if/#elif", t->string);
				error = 1;
				break;
			}
			}
			if (!error && !negativevalue)
			{
				AllocOperator(o);
				o->oper = t->subtype;
				o->priority = PC_OperatorPriority(t->subtype);
				o->parentheses = parentheses;
				o->next = NULL;
				o->prev = lastoperator;
				if (lastoperator)
				{
					lastoperator->next = o;
				}
				else
				{
					firstoperator = o;
				}
				lastoperator = o;
				lastwasvalue = 0;
			}	
			break;
		}
		default:
		{
			SourceError(source, "unknown %s in #if/#elif", t->string);
			error = 1;
			break;
		}
		}
		if (error)
		{
			break;
		}
	}
	if (!error)
	{
		if (!lastwasvalue)
		{
			SourceError(source, "trailing operator in #if/#elif");
			error = 1;
		}
		else if (parentheses)
		{
			SourceError(source, "too many ( in #if/#elif");
			error = 1;
		}
	}

	gotquestmarkvalue = false;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;
	//while there are operators
	while (!error && firstoperator)
	{
		v = firstvalue;
		for (o = firstoperator; o->next; o = o->next)
		{
			//if the current operator is nested deeper in parentheses
			//than the next operator
			if (o->parentheses > o->next->parentheses)
			{
				break;
			}
			//if the current and next operator are nested equally deep in parentheses
			if (o->parentheses == o->next->parentheses)
			{
				//if the priority of the current operator is equal or higher
				//than the priority of the next operator
				if (o->priority >= o->next->priority)
				{
					break;
				}
			}
				//if the arity of the operator isn't equal to 1
			if (o->oper != P_LOGIC_NOT &&
				o->oper != P_BIN_NOT)
			{
				v = v->next;
			}
			//if there's no value or no next value
			if (!v)
			{
				SourceError(source, "mising values in #if/#elif");
				error = 1;
				break;
			}
		}
		if (error)
		{
			break;
		}
		v1 = v;
		v2 = v->next;
		switch (o->oper)
		{
		case P_LOGIC_NOT:       v1->intvalue = !v1->intvalue;
			v1->floatvalue = !v1->floatvalue; break;
		case P_BIN_NOT:         v1->intvalue = ~v1->intvalue;
			break;
		case P_MUL:             v1->intvalue *= v2->intvalue;
			v1->floatvalue *= v2->floatvalue; break;
		case P_DIV:             if (!v2->intvalue || !v2->floatvalue)
			{
				SourceError(source, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			v1->intvalue /= v2->intvalue;
			v1->floatvalue /= v2->floatvalue; break;
		case P_MOD:             if (!v2->intvalue)
			{
				SourceError(source, "divide by zero in #if/#elif\n");
				error = 1;
				break;
			}
			v1->intvalue %= v2->intvalue; break;
		case P_ADD:             v1->intvalue += v2->intvalue;
			v1->floatvalue += v2->floatvalue; break;
		case P_SUB:             v1->intvalue -= v2->intvalue;
			v1->floatvalue -= v2->floatvalue; break;
		case P_LOGIC_AND:       v1->intvalue = v1->intvalue && v2->intvalue;
			v1->floatvalue = v1->floatvalue && v2->floatvalue; break;
		case P_LOGIC_OR:        v1->intvalue = v1->intvalue || v2->intvalue;
			v1->floatvalue = v1->floatvalue || v2->floatvalue; break;
		case P_LOGIC_GEQ:       v1->intvalue = v1->intvalue >= v2->intvalue;
			v1->floatvalue = v1->floatvalue >= v2->floatvalue; break;
		case P_LOGIC_LEQ:       v1->intvalue = v1->intvalue <= v2->intvalue;
			v1->floatvalue = v1->floatvalue <= v2->floatvalue; break;
		case P_LOGIC_EQ:        v1->intvalue = v1->intvalue == v2->intvalue;
			v1->floatvalue = v1->floatvalue == v2->floatvalue; break;
		case P_LOGIC_UNEQ:      v1->intvalue = v1->intvalue != v2->intvalue;
			v1->floatvalue = v1->floatvalue != v2->floatvalue; break;
		case P_LOGIC_GREATER:   v1->intvalue = v1->intvalue > v2->intvalue;
			v1->floatvalue = v1->floatvalue > v2->floatvalue; break;
		case P_LOGIC_LESS:      v1->intvalue = v1->intvalue < v2->intvalue;
			v1->floatvalue = v1->floatvalue < v2->floatvalue; break;
		case P_RSHIFT:          v1->intvalue >>= v2->intvalue;
			break;
		case P_LSHIFT:          v1->intvalue <<= v2->intvalue;
			break;
		case P_BIN_AND:         v1->intvalue &= v2->intvalue;
			break;
		case P_BIN_OR:          v1->intvalue |= v2->intvalue;
			break;
		case P_BIN_XOR:         v1->intvalue ^= v2->intvalue;
			break;
		case P_COLON:
		{
			if (!gotquestmarkvalue)
			{
				SourceError(source, ": without ? in #if/#elif");
				error = 1;
				break;
			}	
			if (integer)
			{
				if (!questmarkintvalue)
				{
					v1->intvalue = v2->intvalue;
				}
			}	
			else
			{
				if (!questmarkfloatvalue)
				{
					v1->floatvalue = v2->floatvalue;
				}
			}
			gotquestmarkvalue = false;
			break;
		}
		case P_QUESTIONMARK:
		{
			if (gotquestmarkvalue)
			{
				SourceError(source, "? after ? in #if/#elif");
				error = 1;
				break;
			}	
			questmarkintvalue = v1->intvalue;
			questmarkfloatvalue = v1->floatvalue;
			gotquestmarkvalue = true;
			break;
		}	
		}
		if (error)
		{
			break;
		}
		lastoperatortype = o->oper;
		//if not an operator with arity 1
		if (o->oper != P_LOGIC_NOT &&
			o->oper != P_BIN_NOT)
		{
			//remove the second value if not question mark operator
			if (o->oper != P_QUESTIONMARK)
			{
				v = v->next;
			}
			//
			if (v->prev)
			{
				v->prev->next = v->next;
			}
			else
			{
				firstvalue = v->next;
			}
			if (v->next)
			{
				v->next->prev = v->prev;
			}
			else
			{
				lastvalue = v->prev;
			}
			FreeValue(v);
		}
		//remove the operator
		if (o->prev)
		{
			o->prev->next = o->next;
		}
		else
		{
			firstoperator = o->next;
		}
		if (o->next)
		{
			o->next->prev = o->prev;
		}
		else
		{
			lastoperator = o->prev;
		}
		FreeOperator(o);
	}
	if (firstvalue)
	{
		if (intvalue)
		{
			*intvalue = firstvalue->intvalue;
		}
		if (floatvalue)
		{
			*floatvalue = firstvalue->floatvalue;
		}
	}
	for (o = firstoperator; o; o = lastoperator)
	{
		lastoperator = o->next;
		FreeOperator(o);
	}
	for (v = firstvalue; v; v = lastvalue)
	{
		lastvalue = v->next;
		FreeValue(v);
	}
	if (!error)
	{
		return true;
	}
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	return false;
}

static bool PC_Evaluate(source_t* source, int* intvalue,
	double* floatvalue, bool integer)
{
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}

	token_t token;
	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "no value after #if/#elif");
		return false;
	}
	token_t* firsttoken = NULL;
	token_t* lasttoken = NULL;
	bool defined = false;
	do
	{
		//if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = false;
				token_t* t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if (!String::Cmp(token.string, "defined"))
			{
				defined = true;
				token_t* t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define_t* define = PC_FindHashedDefine(source->definehash, token.string);
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return false;
				}
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			token_t* t = PC_CopyToken(&token);
			t->next = NULL;
			if (lasttoken)
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else//can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return false;
		}
	}
	while (PC_ReadLine(source, &token));

	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return false;
	}

	token_t* nexttoken;
	for (token_t* t = firsttoken; t; t = nexttoken)
	{
		nexttoken = t->next;
		PC_FreeToken(t);
	}

	return true;
}

static bool PC_DollarEvaluate(source_t* source, int* intvalue,
	double* floatvalue, bool integer)
{
	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}

	token_t token;
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "no leading ( after $evalint/$evalfloat");
		return false;
	}
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "nothing to evaluate");
		return false;
	}
	int indent = 1;
	token_t* firsttoken = NULL;
	token_t* lasttoken = NULL;
	bool defined = false;
	do
	{
		//if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = false;
				token_t* t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else if (!String::Cmp(token.string, "defined"))
			{
				defined = true;
				token_t* t = PC_CopyToken(&token);
				t->next = NULL;
				if (lasttoken)
				{
					lasttoken->next = t;
				}
				else
				{
					firsttoken = t;
				}
				lasttoken = t;
			}
			else
			{
				//then it must be a define
				define_t* define = PC_FindHashedDefine(source->definehash, token.string);
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return false;
				}
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			if (*token.string == '(')
			{
				indent++;
			}
			else if (*token.string == ')')
			{
				indent--;
			}
			if (indent <= 0)
			{
				break;
			}
			token_t* t = PC_CopyToken(&token);
			t->next = NULL;
			if (lasttoken)
			{
				lasttoken->next = t;
			}
			else
			{
				firsttoken = t;
			}
			lasttoken = t;
		}
		else//can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return false;
		}
	}
	while (PC_ReadSourceToken(source, &token));

	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return false;
	}

	token_t * nexttoken;
	for (token_t* t = firsttoken; t; t = nexttoken)
	{
		nexttoken = t->next;
		PC_FreeToken(t);
	}

	return true;
}

static bool PC_Directive_if(source_t* source)
{
	int value;
	if (!PC_Evaluate(source, &value, NULL, true))
	{
		return false;
	}
	int skip = (value == 0);
	PC_PushIndent(source, INDENT_IF, skip);
	return true;
}

static bool PC_Directive_elif(source_t* source)
{
	int type, skip;
	PC_PopIndent(source, &type, &skip);
	if (!type || type == INDENT_ELSE)
	{
		SourceError(source, "misplaced #elif");
		return false;
	}
	int value;
	if (!PC_Evaluate(source, &value, NULL, true))
	{
		return false;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_ELIF, skip);
	return true;
}

static bool PC_Directive_line(source_t* source)
{
	SourceError(source, "#line directive not supported");
	return false;
}

static bool PC_Directive_error(source_t* source)
{
	token_t token;
	String::Cpy(token.string, "");
	PC_ReadSourceToken(source, &token);
	SourceError(source, "#error directive: %s", token.string);
	return false;
}

static bool PC_Directive_pragma(source_t* source)
{
	SourceWarning(source, "#pragma directive not supported");
	token_t token;
	while (PC_ReadLine(source, &token))
		;
	return true;
}

static void UnreadSignToken(source_t* source)
{
	token_t token;
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	String::Cpy(token.string, "-");
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	PC_UnreadSourceToken(source, &token);
}

static bool PC_Directive_eval(source_t* source)
{
	int value;
	if (!PC_Evaluate(source, &value, NULL, true))
	{
		return false;
	}

	token_t token;
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%d", abs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return true;
}

static bool PC_Directive_evalfloat(source_t* source)
{
	double value;
	if (!PC_Evaluate(source, NULL, &value, false))
	{
		return false;
	}
	token_t token;
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%1.2f", Q_fabs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return true;
}

static directive_t directives[] =
{
	{"if", PC_Directive_if},
	{"ifdef", PC_Directive_ifdef},
	{"ifndef", PC_Directive_ifndef},
	{"elif", PC_Directive_elif},
	{"else", PC_Directive_else},
	{"endif", PC_Directive_endif},
	{"include", PC_Directive_include},
	{"define", PC_Directive_define},
	{"undef", PC_Directive_undef},
	{"line", PC_Directive_line},
	{"error", PC_Directive_error},
	{"pragma", PC_Directive_pragma},
	{"eval", PC_Directive_eval},
	{"evalfloat", PC_Directive_evalfloat},
	{NULL, NULL}
};

static bool PC_ReadDirective(source_t* source)
{
	//read the directive name
	token_t token;
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found # without name");
		return false;
	}
	//directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found # at end of line");
		return false;
	}
	//if if is a name
	if (token.type == TT_NAME)
	{
		//find the precompiler directive
		for (int i = 0; directives[i].name; i++)
		{
			if (!String::Cmp(directives[i].name, token.string))
			{
				return directives[i].func(source);
			}
		}
	}
	SourceError(source, "unknown precompiler directive %s", token.string);
	return false;
}

static bool PC_DollarDirective_evalint(source_t* source)
{
	int value;
	if (!PC_DollarEvaluate(source, &value, NULL, true))
	{
		return false;
	}

	token_t token;
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%d", abs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL;
	token.intvalue = value;
	token.floatvalue = value;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return true;
}

static bool PC_DollarDirective_evalfloat(source_t* source)
{
	double value;
	if (!PC_DollarEvaluate(source, NULL, &value, false))
	{
		return false;
	}

	token_t token;
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%1.2f", Q_fabs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	token.intvalue = (unsigned long)value;
	token.floatvalue = value;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return true;
}

static directive_t dollardirectives[20] =
{
	{"evalint", PC_DollarDirective_evalint},
	{"evalfloat", PC_DollarDirective_evalfloat},
	{NULL, NULL}
};

static bool PC_ReadDollarDirective(source_t* source)
{
	//read the directive name
	token_t token;
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found $ without name");
		return false;
	}
	//directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found $ at end of line");
		return false;
	}
	//if if is a name
	if (token.type == TT_NAME)
	{
		//find the precompiler directive
		for (int i = 0; dollardirectives[i].name; i++)
		{
			if (!String::Cmp(dollardirectives[i].name, token.string))
			{
				return dollardirectives[i].func(source);
			}
		}
	}
	PC_UnreadSourceToken(source, &token);
	SourceError(source, "unknown precompiler directive %s", token.string);
	return false;
}

int PC_ReadToken(source_t* source, token_t* token)
{
	define_t* define;

	while (1)
	{
		if (!PC_ReadSourceToken(source, token))
		{
			return false;
		}
		//check for precompiler directives
		if (token->type == TT_PUNCTUATION && *token->string == '#')
		{
			//read the precompiler directive
			if (!PC_ReadDirective(source))
			{
				return false;
			}
			continue;
		}	//end if
		if (token->type == TT_PUNCTUATION && *token->string == '$')
		{
			//read the precompiler directive
			if (!PC_ReadDollarDirective(source))
			{
				return false;
			}
			continue;
		}	//end if
		//if skipping source because of conditional compilation
		if (source->skip)
		{
			continue;
		}
		// recursively concatenate strings that are behind each other still resolving defines
		if (token->type == TT_STRING)
		{
			token_t newtoken;
			if (PC_ReadToken(source, &newtoken))
			{
				if (newtoken.type == TT_STRING)
				{
					token->string[String::Length(token->string) - 1] = '\0';
					if (String::Length(token->string) + String::Length(newtoken.string + 1) + 1 >= MAX_TOKEN)
					{
						SourceError(source, "string longer than MAX_TOKEN %d\n", MAX_TOKEN);
						return false;
					}
					String::Cat(token->string, MAX_TOKEN, newtoken.string + 1);
				}
				else
				{
					PC_UnreadSourceToken(source, &newtoken);
				}
			}
		}	//end if
		//if the token is a name
		if (token->type == TT_NAME)
		{
			//check if the name is a define macro
			define = PC_FindHashedDefine(source->definehash, token->string);
			//if it is a define macro
			if (define)
			{
				//expand the defined macro
				if (!PC_ExpandDefineIntoSource(source, token, define))
				{
					return false;
				}
				continue;
			}	//end if
		}	//end if
			//copy token for unreading
		Com_Memcpy(&source->token, token, sizeof(token_t));
		//found a token
		return true;
	}	//end while
}	//end of the function PC_ReadToken
