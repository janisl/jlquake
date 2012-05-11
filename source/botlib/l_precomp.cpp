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
#include "l_script.h"
#include "l_precomp.h"

#define MAX_DEFINEPARMS         128

//directive name with parse function
typedef struct directive_s
{
	const char* name;
	bool (* func)(source_t* source);
} directive_t;

//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ReadDefineParms(source_t* source, define_t* define, token_t** parms, int maxparms)
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
	for (done = 0, numparms = 0, indent = 0; !done; )
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
				if (indent <= 0)
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
				continue;
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
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_StringizeTokens(token_t* tokens, token_t* token)
{
	token_t* t;

	token->type = TT_STRING;
	token->whitespace_p = NULL;
	token->endwhitespace_p = NULL;
	token->string[0] = '\0';
	String::Cat(token->string, MAX_TOKEN, "\"");
	for (t = tokens; t; t = t->next)
	{
		String::Cat(token->string, MAX_TOKEN, t->string);
	}	//end for
	String::Cat(token->string, MAX_TOKEN, "\"");
	return true;
}	//end of the function PC_StringizeTokens
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_MergeTokens(token_t* t1, token_t* t2)
{
	//merging of a name with a name or number
	if (t1->type == TT_NAME && (t2->type == TT_NAME || t2->type == TT_NUMBER))
	{
		String::Cat(t1->string, MAX_TOKEN, t2->string);
		return true;
	}	//end if
		//merging of two strings
	if (t1->type == TT_STRING && t2->type == TT_STRING)
	{
		//remove trailing double quote
		t1->string[String::Length(t1->string) - 1] = '\0';
		//concat without leading double quote
		String::Cat(t1->string, MAX_TOKEN, &t2->string[1]);
		return true;
	}	//end if
		//FIXME: merging of two number of the same sub type
	return false;
}	//end of the function PC_MergeTokens
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
define_t* PC_FindDefine(define_t* defines, char* name)
{
	define_t* d;

	for (d = defines; d; d = d->next)
	{
		if (!String::Cmp(d->name, name))
		{
			return d;
		}
	}	//end for
	return NULL;
}	//end of the function PC_FindDefine
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PC_AddBuiltinDefines(source_t* source)
{
	int i;
	define_t* define;
	struct builtin_t
	{
		const char* string;
		int builtin;
	} builtin[] = {	// bk001204 - brackets
		{ "__LINE__",   BUILTIN_LINE },
		{ "__FILE__",   BUILTIN_FILE },
		{ "__DATE__",   BUILTIN_DATE },
		{ "__TIME__",   BUILTIN_TIME },
//		{ "__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};

	for (i = 0; builtin[i].string; i++)
	{
		define = (define_t*)Mem_Alloc(sizeof(define_t) + String::Length(builtin[i].string) + 1);
		Com_Memset(define, 0, sizeof(define_t));
		define->name = (char*)define + sizeof(define_t);
		String::Cpy(define->name, builtin[i].string);
		define->flags |= DEFINE_FIXED;
		define->builtin = builtin[i].builtin;
		//add the define to the source
		PC_AddDefineToHash(define, source->definehash);
	}	//end for
}	//end of the function PC_AddBuiltinDefines
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandBuiltinDefine(source_t* source, token_t* deftoken, define_t* define,
	token_t** firsttoken, token_t** lasttoken)
{
	token_t* token;
	unsigned long t;	//	time_t t; //to prevent LCC warning
	char* curtime;

	token = PC_CopyToken(deftoken);
	switch (define->builtin)
	{
	case BUILTIN_LINE:
	{
		sprintf(token->string, "%d", deftoken->line);
		token->intvalue = deftoken->line;
		token->floatvalue = deftoken->line;
		token->type = TT_NUMBER;
		token->subtype = TT_DECIMAL | TT_INTEGER;
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_FILE:
	{
		String::Cpy(token->string, source->scriptstack->filename);
		token->type = TT_NAME;
		token->subtype = String::Length(token->string);
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_DATE:
	{
		t = time(NULL);
		curtime = ctime((time_t*)&t);
		String::Cpy(token->string, "\"");
		strncat(token->string, curtime + 4, 7);
		strncat(token->string + 7, curtime + 20, 4);
		String::Cat(token->string, MAX_TOKEN, "\"");
		free(curtime);
		token->type = TT_NAME;
		token->subtype = String::Length(token->string);
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_TIME:
	{
		t = time(NULL);
		curtime = ctime((time_t*)&t);
		String::Cpy(token->string, "\"");
		strncat(token->string, curtime + 11, 8);
		String::Cat(token->string, MAX_TOKEN, "\"");
		free(curtime);
		token->type = TT_NAME;
		token->subtype = String::Length(token->string);
		*firsttoken = token;
		*lasttoken = token;
		break;
	}		//end case
	case BUILTIN_STDC:
	default:
	{
		*firsttoken = NULL;
		*lasttoken = NULL;
		break;
	}		//end case
	}	//end switch
	return true;
}	//end of the function PC_ExpandBuiltinDefine
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandDefine(source_t* source, token_t* deftoken, define_t* define,
	token_t** firsttoken, token_t** lasttoken)
{
	token_t* parms[MAX_DEFINEPARMS], * dt, * pt, * t;
	token_t* t1, * t2, * first, * last, * nextpt, token;
	int parmnum, i;

	//if it is a builtin define
	if (define->builtin)
	{
		return PC_ExpandBuiltinDefine(source, deftoken, define, firsttoken, lasttoken);
	}	//end if
		//if the define has parameters
	if (define->numparms)
	{
		if (!PC_ReadDefineParms(source, define, parms, MAX_DEFINEPARMS))
		{
			return false;
		}
	}	//end if
		//empty list at first
	first = NULL;
	last = NULL;
	//create a list with tokens of the expanded define
	for (dt = define->tokens; dt; dt = dt->next)
	{
		parmnum = -1;
		//if the token is a name, it could be a define parameter
		if (dt->type == TT_NAME)
		{
			parmnum = PC_FindDefineParm(define, dt->string);
		}	//end if
			//if it is a define parameter
		if (parmnum >= 0)
		{
			for (pt = parms[parmnum]; pt; pt = pt->next)
			{
				t = PC_CopyToken(pt);
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
			}	//end for
		}	//end if
		else
		{
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
					if (!PC_StringizeTokens(parms[parmnum], &token))
					{
						SourceError(source, "can't stringize tokens");
						return false;
					}	//end if
					t = PC_CopyToken(&token);
				}	//end if
				else
				{
					SourceWarning(source, "stringizing operator without define parameter");
					continue;
				}	//end if
			}	//end if
			else
			{
				t = PC_CopyToken(dt);
			}	//end else
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
		}	//end else
	}	//end for
		//check for the merging operator
	for (t = first; t; )
	{
		if (t->next)
		{
			//if the merging operator
			if (t->next->string[0] == '#' && t->next->string[1] == '#')
			{
				t1 = t;
				t2 = t->next->next;
				if (t2)
				{
					if (!PC_MergeTokens(t1, t2))
					{
						SourceError(source, "can't merge %s with %s", t1->string, t2->string);
						return false;
					}	//end if
					PC_FreeToken(t1->next);
					t1->next = t2->next;
					if (t2 == last)
					{
						last = t1;
					}
					PC_FreeToken(t2);
					continue;
				}	//end if
			}	//end if
		}	//end if
		t = t->next;
	}	//end for
		//store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;
	//free all the parameter tokens
	for (i = 0; i < define->numparms; i++)
	{
		for (pt = parms[i]; pt; pt = nextpt)
		{
			nextpt = pt->next;
			PC_FreeToken(pt);
		}	//end for
	}	//end for
		//
	return true;
}	//end of the function PC_ExpandDefine
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_ExpandDefineIntoSource(source_t* source, token_t* deftoken, define_t* define)
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
	}	//end if
	return false;
}	//end of the function PC_ExpandDefineIntoSource
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_Evaluate(source_t* source, int* intvalue,
	double* floatvalue, int integer)
{
	token_t token, * firsttoken, * lasttoken;
	token_t* t, * nexttoken;
	define_t* define;
	int defined = false;

	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	//
	if (!PC_ReadLine(source, &token))
	{
		SourceError(source, "no value after #if/#elif");
		return false;
	}	//end if
	firsttoken = NULL;
	lasttoken = NULL;
	do
	{
		//if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = false;
				t = PC_CopyToken(&token);
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
			}	//end if
			else if (!String::Cmp(token.string, "defined"))
			{
				defined = true;
				t = PC_CopyToken(&token);
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
			}	//end if
			else
			{
				//then it must be a define
				define = PC_FindHashedDefine(source->definehash, token.string);
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return false;
				}	//end if
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return false;
				}
			}	//end else
		}	//end if
			//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION)
		{
			t = PC_CopyToken(&token);
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
		}	//end else
		else//can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return false;
		}	//end else
	}
	while (PC_ReadLine(source, &token));
	//
	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return false;
	}
	//
	for (t = firsttoken; t; t = nexttoken)
	{
		nexttoken = t->next;
		PC_FreeToken(t);
	}	//end for
		//
	return true;
}	//end of the function PC_Evaluate
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PC_DollarEvaluate(source_t* source, int* intvalue,
	double* floatvalue, int integer)
{
	int indent, defined = false;
	token_t token, * firsttoken, * lasttoken;
	token_t* t, * nexttoken;
	define_t* define;

	if (intvalue)
	{
		*intvalue = 0;
	}
	if (floatvalue)
	{
		*floatvalue = 0;
	}
	//
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "no leading ( after $evalint/$evalfloat");
		return false;
	}	//end if
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "nothing to evaluate");
		return false;
	}	//end if
	indent = 1;
	firsttoken = NULL;
	lasttoken = NULL;
	do
	{
		//if the token is a name
		if (token.type == TT_NAME)
		{
			if (defined)
			{
				defined = false;
				t = PC_CopyToken(&token);
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
			}	//end if
			else if (!String::Cmp(token.string, "defined"))
			{
				defined = true;
				t = PC_CopyToken(&token);
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
			}	//end if
			else
			{
				//then it must be a define
				define = PC_FindHashedDefine(source->definehash, token.string);
				if (!define)
				{
					SourceError(source, "can't evaluate %s, not defined", token.string);
					return false;
				}	//end if
				if (!PC_ExpandDefineIntoSource(source, &token, define))
				{
					return false;
				}
			}	//end else
		}	//end if
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
			t = PC_CopyToken(&token);
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
		}	//end else
		else//can't evaluate the token
		{
			SourceError(source, "can't evaluate %s", token.string);
			return false;
		}	//end else
	}
	while (PC_ReadSourceToken(source, &token));
	//
	if (!PC_EvaluateTokens(source, firsttoken, intvalue, floatvalue, integer))
	{
		return false;
	}
	//
	for (t = firsttoken; t; t = nexttoken)
	{
		nexttoken = t->next;
		PC_FreeToken(t);
	}	//end for
		//
	return true;
}	//end of the function PC_DollarEvaluate
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_elif(source_t* source)
{
	int value;
	int type, skip;

	PC_PopIndent(source, &type, &skip);
	if (!type || type == INDENT_ELSE)
	{
		SourceError(source, "misplaced #elif");
		return false;
	}	//end if
	if (!PC_Evaluate(source, &value, NULL, true))
	{
		return false;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_ELIF, skip);
	return true;
}	//end of the function PC_Directive_elif
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_if(source_t* source)
{
	int value;
	int skip;

	if (!PC_Evaluate(source, &value, NULL, true))
	{
		return false;
	}
	skip = (value == 0);
	PC_PushIndent(source, INDENT_IF, skip);
	return true;
}	//end of the function PC_Directive
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_line(source_t* source)
{
	SourceError(source, "#line directive not supported");
	return false;
}	//end of the function PC_Directive_line
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_error(source_t* source)
{
	token_t token;

	String::Cpy(token.string, "");
	PC_ReadSourceToken(source, &token);
	SourceError(source, "#error directive: %s", token.string);
	return false;
}	//end of the function PC_Directive_error
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_pragma(source_t* source)
{
	token_t token;

	SourceWarning(source, "#pragma directive not supported");
	while (PC_ReadLine(source, &token))
		;
	return true;
}	//end of the function PC_Directive_pragma
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void UnreadSignToken(source_t* source)
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
}	//end of the function UnreadSignToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_eval(source_t* source)
{
	int value;
	token_t token;

	if (!PC_Evaluate(source, &value, NULL, true))
	{
		return false;
	}
	//
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
}	//end of the function PC_Directive_eval
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_Directive_evalfloat(source_t* source)
{
	double value;
	token_t token;

	if (!PC_Evaluate(source, NULL, &value, false))
	{
		return false;
	}
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%1.2f", fabs(value));
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL;
	PC_UnreadSourceToken(source, &token);
	if (value < 0)
	{
		UnreadSignToken(source);
	}
	return true;
}	//end of the function PC_Directive_evalfloat
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
directive_t directives[20] =
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

int PC_ReadDirective(source_t* source)
{
	token_t token;
	int i;

	//read the directive name
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found # without name");
		return false;
	}	//end if
		//directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found # at end of line");
		return false;
	}	//end if
		//if if is a name
	if (token.type == TT_NAME)
	{
		//find the precompiler directive
		for (i = 0; directives[i].name; i++)
		{
			if (!String::Cmp(directives[i].name, token.string))
			{
				return directives[i].func(source);
			}	//end if
		}	//end for
	}	//end if
	SourceError(source, "unknown precompiler directive %s", token.string);
	return false;
}	//end of the function PC_ReadDirective
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_DollarDirective_evalint(source_t* source)
{
	int value;
	token_t token;

	if (!PC_DollarEvaluate(source, &value, NULL, true))
	{
		return false;
	}
	//
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
}	//end of the function PC_DollarDirective_evalint
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
bool PC_DollarDirective_evalfloat(source_t* source)
{
	double value;
	token_t token;

	if (!PC_DollarEvaluate(source, NULL, &value, false))
	{
		return false;
	}
	token.line = source->scriptstack->line;
	token.whitespace_p = source->scriptstack->script_p;
	token.endwhitespace_p = source->scriptstack->script_p;
	token.linescrossed = 0;
	sprintf(token.string, "%1.2f", fabs(value));
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
}	//end of the function PC_DollarDirective_evalfloat
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
directive_t dollardirectives[20] =
{
	{"evalint", PC_DollarDirective_evalint},
	{"evalfloat", PC_DollarDirective_evalfloat},
	{NULL, NULL}
};

int PC_ReadDollarDirective(source_t* source)
{
	token_t token;
	int i;

	//read the directive name
	if (!PC_ReadSourceToken(source, &token))
	{
		SourceError(source, "found $ without name");
		return false;
	}	//end if
		//directive name must be on the same line
	if (token.linescrossed > 0)
	{
		PC_UnreadSourceToken(source, &token);
		SourceError(source, "found $ at end of line");
		return false;
	}	//end if
		//if if is a name
	if (token.type == TT_NAME)
	{
		//find the precompiler directive
		for (i = 0; dollardirectives[i].name; i++)
		{
			if (!String::Cmp(dollardirectives[i].name, token.string))
			{
				return dollardirectives[i].func(source);
			}	//end if
		}	//end for
	}	//end if
	PC_UnreadSourceToken(source, &token);
	SourceError(source, "unknown precompiler directive %s", token.string);
	return false;
}	//end of the function PC_ReadDirective

//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
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
					PC_UnreadToken(source, &newtoken);
				}
			}
		}	//end if
			//if skipping source because of conditional compilation
		if (source->skip)
		{
			continue;
		}
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
