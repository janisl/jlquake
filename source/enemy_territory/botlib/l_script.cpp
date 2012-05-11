/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		l_script.c
 *
 * desc:		lexicographical parser
 *
 *
 *****************************************************************************/

//include files for usage in the bot library
#include "../game/q_shared.h"
#include "l_script.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
const char* PunctuationFromNum(script_t* script, int num)
{
	int i;

	for (i = 0; script->punctuations[i].p; i++)
	{
		if (script->punctuations[i].n == num)
		{
			return script->punctuations[i].p;
		}
	}	//end for
	return "unkown punctuation";
}	//end of the function PunctuationFromNum
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PS_ExpectTokenString(script_t* script, char* string)
{
	token_t token;

	if (!PS_ReadToken(script, &token))
	{
		ScriptError(script, "couldn't find expected %s", string);
		return 0;
	}	//end if

	if (String::Cmp(token.string, string))
	{
		ScriptError(script, "expected %s, found %s", string, token.string);
		return 0;
	}	//end if
	return 1;
}	//end of the function PS_ExpectToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PS_ExpectTokenType(script_t* script, int type, int subtype, token_t* token)
{
	char str[MAX_TOKEN];

	if (!PS_ReadToken(script, token))
	{
		ScriptError(script, "couldn't read expected token");
		return 0;
	}	//end if

	if (token->type != type)
	{
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
		ScriptError(script, "expected a %s, found %s", str, token->string);
		return 0;
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
				strcat(str, " long");
			}
			if (subtype & TT_UNSIGNED)
			{
				strcat(str, " unsigned");
			}
			if (subtype & TT_FLOAT)
			{
				strcat(str, " float");
			}
			if (subtype & TT_INTEGER)
			{
				strcat(str, " integer");
			}
			ScriptError(script, "expected %s, found %s", str, token->string);
			return 0;
		}	//end if
	}	//end if
	else if (token->type == TT_PUNCTUATION)
	{
		if (subtype < 0)
		{
			ScriptError(script, "BUG: wrong punctuation subtype");
			return 0;
		}	//end if
		if (token->subtype != subtype)
		{
			ScriptError(script, "expected %s, found %s",
				script->punctuations[subtype], token->string);
			return 0;
		}	//end if
	}	//end else if
	return 1;
}	//end of the function PS_ExpectTokenType
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PS_ExpectAnyToken(script_t* script, token_t* token)
{
	if (!PS_ReadToken(script, token))
	{
		ScriptError(script, "couldn't read expected token");
		return 0;
	}	//end if
	else
	{
		return 1;
	}	//end else
}	//end of the function PS_ExpectAnyToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PS_CheckTokenString(script_t* script, char* string)
{
	token_t tok;

	if (!PS_ReadToken(script, &tok))
	{
		return 0;
	}
	//if the token is available
	if (!String::Cmp(tok.string, string))
	{
		return 1;
	}
	//token not available
	script->script_p = script->lastscript_p;
	return 0;
}	//end of the function PS_CheckTokenString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PS_CheckTokenType(script_t* script, int type, int subtype, token_t* token)
{
	token_t tok;

	if (!PS_ReadToken(script, &tok))
	{
		return 0;
	}
	//if the type matches
	if (tok.type == type &&
		(tok.subtype & subtype) == subtype)
	{
		memcpy(token, &tok, sizeof(token_t));
		return 1;
	}	//end if
		//token is not available
	script->script_p = script->lastscript_p;
	return 0;
}	//end of the function PS_CheckTokenType
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int PS_SkipUntilString(script_t* script, char* string)
{
	token_t token;

	while (PS_ReadToken(script, &token))
	{
		if (!String::Cmp(token.string, string))
		{
			return 1;
		}
	}	//end while
	return 0;
}	//end of the function PS_SkipUntilString
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PS_UnreadLastToken(script_t* script)
{
	script->tokenavailable = 1;
}	//end of the function UnreadLastToken
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void PS_UnreadToken(script_t* script, token_t* token)
{
	memcpy(&script->token, token, sizeof(token_t));
	script->tokenavailable = 1;
}	//end of the function UnreadToken
//============================================================================
// returns the next character of the read white space, returns NULL if none
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
char PS_NextWhiteSpaceChar(script_t* script)
{
	if (script->whitespace_p != script->endwhitespace_p)
	{
		return *script->whitespace_p++;
	}	//end if
	else
	{
		return 0;
	}	//end else
}	//end of the function PS_NextWhiteSpaceChar
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
long double ReadSignedFloat(script_t* script)
{
	token_t token;
	long double sign = 1;

	PS_ExpectAnyToken(script, &token);
	if (!String::Cmp(token.string, "-"))
	{
		sign = -1;
		PS_ExpectTokenType(script, TT_NUMBER, 0, &token);
	}	//end if
	else if (token.type != TT_NUMBER)
	{
		ScriptError(script, "expected float value, found %s\n", token.string);
	}	//end else if
	return sign * token.floatvalue;
}	//end of the function ReadSignedFloat
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
signed long int ReadSignedInt(script_t* script)
{
	token_t token;
	signed long int sign = 1;

	PS_ExpectAnyToken(script, &token);
	if (!String::Cmp(token.string, "-"))
	{
		sign = -1;
		PS_ExpectTokenType(script, TT_NUMBER, TT_INTEGER, &token);
	}	//end if
	else if (token.type != TT_NUMBER || token.subtype == TT_FLOAT)
	{
		ScriptError(script, "expected integer value, found %s\n", token.string);
	}	//end else if
	return sign * token.intvalue;
}	//end of the function ReadSignedInt
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void SetScriptFlags(script_t* script, int flags)
{
	script->flags = flags;
}	//end of the function SetScriptFlags
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int GetScriptFlags(script_t* script)
{
	return script->flags;
}	//end of the function GetScriptFlags
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
void ResetScript(script_t* script)
{
	//pointer in script buffer
	script->script_p = script->buffer;
	//pointer in script buffer before reading token
	script->lastscript_p = script->buffer;
	//begin of white space
	script->whitespace_p = NULL;
	//end of white space
	script->endwhitespace_p = NULL;
	//set if there's a token available in script->token
	script->tokenavailable = 0;
	//
	script->line = 1;
	script->lastline = 1;
	//clear the saved token
	memset(&script->token, 0, sizeof(token_t));
}	//end of the function ResetScript
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int NumLinesCrossed(script_t* script)
{
	return script->line - script->lastline;
}	//end of the function NumLinesCrossed
//============================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//============================================================================
int ScriptSkipTo(script_t* script, char* value)
{
	int len;
	char firstchar;

	firstchar = *value;
	len = String::Length(value);
	do
	{
		if (!PS_ReadWhiteSpace(script))
		{
			return 0;
		}
		if (*script->script_p == firstchar)
		{
			if (!String::NCmp(script->script_p, value, len))
			{
				return 1;
			}	//end if
		}	//end if
		script->script_p++;
	}
	while (1);
}	//end of the function ScriptSkipTo
