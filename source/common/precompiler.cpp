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

bool PC_Directive_ifdef(source_t* source)
{
	return PC_Directive_if_def(source, INDENT_IFDEF);
}

bool PC_Directive_ifndef(source_t* source)
{
	return PC_Directive_if_def(source, INDENT_IFNDEF);
}

bool PC_Directive_else(source_t* source)
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

bool PC_Directive_endif(source_t* source)
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

bool PC_ReadDefineParms(source_t* source, define_t* define, token_t** parms, int maxparms)
{
	token_t token, * t, * last;
	int i, done, lastcomma, numparms, indent;

	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "define %s missing parms", define->name);
		return false;
	}	//end if
		//
	if (define->numparms > maxparms)
	{
		SourceError(source, "define with more than %d parameters", maxparms);
		return false;
	}	//end if
		//
	for (i = 0; i < define->numparms; i++)
		parms[i] = NULL;
	//if no leading "("
	if (String::Cmp(token.string, "("))
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "define %s missing parms", define->name);
		return false;
	}	//end if
		//read the define parameters
	for (done = 0, numparms = 0, indent = 1; !done; )
	{
		if (numparms >= maxparms)
		{
			SourceError(source, "define %s with too many parms", define->name);
			return false;
		}	//end if
		if (numparms >= define->numparms)
		{
			SourceWarning(source, "define %s has too many parms", define->name);
			return false;
		}	//end if
		parms[numparms] = NULL;
		lastcomma = 1;
		last = NULL;
		while (!done)
		{
			//
			if (!PC_ReadSourceToken(source, &token))
			{
				SourceError(source, "define %s incomplete", define->name);
				return false;
			}	//end if
				//
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
				}	//end if
			}	//end if
			lastcomma = 0;
			//
			if (!String::Cmp(token.string, "("))
			{
				indent++;
			}	//end if
			else if (!String::Cmp(token.string, ")"))
			{
				if (--indent <= 0)
				{
					if (!parms[define->numparms - 1])
					{
						SourceWarning(source, "too few define parms");
					}	//end if
					done = 1;
					break;
				}	//end if
			}	//end if
				//
			if (numparms < define->numparms)
			{
				//
				t = PC_CopyToken(&token);
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
			}	//end if
		}	//end while
		numparms++;
	}	//end for
	return true;
}	//end of the function PC_ReadDefineParms

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

bool PC_EvaluateTokens(source_t* source, token_t* tokens, int* intvalue,
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
			}		//end if
			if (String::Cmp(t->string, "defined"))
			{
				SourceError(source, "undefined name %s in #if/#elif", t->string);
				error = 1;
				break;
			}		//end if
			t = t->next;
			if (!String::Cmp(t->string, "("))
			{
				brace = true;
				t = t->next;
			}		//end if
			if (!t || t->type != TT_NAME)
			{
				SourceError(source, "defined without name in #if/#elif");
				error = 1;
				break;
			}		//end if
					//v = (value_t *) GetClearedMemory(sizeof(value_t));
			AllocValue(v);
			if (PC_FindHashedDefine(source->definehash, t->string))
			{
				v->intvalue = 1;
				v->floatvalue = 1;
			}		//end if
			else
			{
				v->intvalue = 0;
				v->floatvalue = 0;
			}		//end else
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
				}		//end if
			}		//end if
			brace = false;
			// defined() creates a value
			lastwasvalue = 1;
			break;
		}		//end case
		case TT_NUMBER:
		{
			if (lastwasvalue)
			{
				SourceError(source, "syntax error in #if/#elif");
				error = 1;
				break;
			}		//end if
					//v = (value_t *) GetClearedMemory(sizeof(value_t));
			AllocValue(v);
			if (negativevalue)
			{
				v->intvalue = -(signed int)t->intvalue;
				v->floatvalue = -t->floatvalue;
			}		//end if
			else
			{
				v->intvalue = t->intvalue;
				v->floatvalue = t->floatvalue;
			}		//end else
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
		}		//end case
		case TT_PUNCTUATION:
		{
			if (negativevalue)
			{
				SourceError(source, "misplaced minus sign in #if/#elif");
				error = 1;
				break;
			}		//end if
			if (t->subtype == P_PARENTHESESOPEN)
			{
				parentheses++;
				break;
			}		//end if
			else if (t->subtype == P_PARENTHESESCLOSE)
			{
				parentheses--;
				if (parentheses < 0)
				{
					SourceError(source, "too many ) in #if/#elsif");
					error = 1;
				}		//end if
				break;
			}		//end else if
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
				}		//end if
			}		//end if
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
				}			//end if
				break;
			}			//end case
			case P_INC:
			case P_DEC:
			{
				SourceError(source, "++ or -- used in #if/#elif");
				break;
			}			//end case
			case P_SUB:
			{
				if (!lastwasvalue)
				{
					negativevalue = 1;
					break;
				}			//end if
			}			//end case

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
				}			//end if
				break;
			}			//end case
			default:
			{
				SourceError(source, "invalid operator %s in #if/#elif", t->string);
				error = 1;
				break;
			}			//end default
			}		//end switch
			if (!error && !negativevalue)
			{
				//o = (operator_t *) GetClearedMemory(sizeof(operator_t));
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
			}		//end if
			break;
		}		//end case
		default:
		{
			SourceError(source, "unknown %s in #if/#elif", t->string);
			error = 1;
			break;
		}		//end default
		}	//end switch
		if (error)
		{
			break;
		}
	}	//end for
	if (!error)
	{
		if (!lastwasvalue)
		{
			SourceError(source, "trailing operator in #if/#elif");
			error = 1;
		}	//end if
		else if (parentheses)
		{
			SourceError(source, "too many ( in #if/#elif");
			error = 1;
		}	//end else if
	}	//end if
		//
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
			}	//end if
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
			}	//end if
		}	//end for
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
			}		//end if
			if (integer)
			{
				if (!questmarkintvalue)
				{
					v1->intvalue = v2->intvalue;
				}
			}		//end if
			else
			{
				if (!questmarkfloatvalue)
				{
					v1->floatvalue = v2->floatvalue;
				}
			}		//end else
			gotquestmarkvalue = false;
			break;
		}		//end case
		case P_QUESTIONMARK:
		{
			if (gotquestmarkvalue)
			{
				SourceError(source, "? after ? in #if/#elif");
				error = 1;
				break;
			}		//end if
			questmarkintvalue = v1->intvalue;
			questmarkfloatvalue = v1->floatvalue;
			gotquestmarkvalue = true;
			break;
		}		//end if
		}	//end switch
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
			//FreeMemory(v);
			FreeValue(v);
		}	//end if
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
		//FreeMemory(o);
		FreeOperator(o);
	}	//end while
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
	}	//end if
	for (o = firstoperator; o; o = lastoperator)
	{
		lastoperator = o->next;
		//FreeMemory(o);
		FreeOperator(o);
	}	//end for
	for (v = firstvalue; v; v = lastvalue)
	{
		lastvalue = v->next;
		//FreeMemory(v);
		FreeValue(v);
	}	//end for
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
}	//end of the function PC_EvaluateTokens
