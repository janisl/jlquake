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

#include "../../common/strings.h"
#include "../../common/quake3defs.h"
#include "../../common/files.h"
#include "q_parse.h"

#define MAX_PARSE_INFO  16

struct parseInfo_t
{
	char token[MAX_TOKEN_CHARS_Q3];
	int lines;
	qboolean ungetToken;
	char parseFile[MAX_QPATH];
};

static parseInfo_t parseInfo[MAX_PARSE_INFO];
static int parseInfoNum;
static parseInfo_t* pi = &parseInfo[0];

// multiple character punctuation tokens
static const char* punctuation[] =
{
	"+=", "-=",  "*=",  "/=", "&=", "|=", "++", "--",
	"&&", "||",  "<=",  ">=", "==", "!=",
	NULL
};

void Com_BeginParseSession(const char* filename)
{
	if (parseInfoNum == MAX_PARSE_INFO - 1)
	{
		common->FatalError("Com_BeginParseSession: session overflow");
	}
	parseInfoNum++;
	pi = &parseInfo[parseInfoNum];

	pi->lines = 1;
	String::NCpyZ(pi->parseFile, filename, sizeof(pi->parseFile));
}

void Com_EndParseSession()
{
	if (parseInfoNum == 0)
	{
		common->FatalError("Com_EndParseSession: session underflow");
	}
	parseInfoNum--;
	pi = &parseInfo[parseInfoNum];
}

/*
Prints the script name and line number in the message
*/
void Com_ScriptError(const char* msg, ...)
{
	va_list argptr;
	char string[32000];

	va_start(argptr, msg);
	vsprintf(string, msg,argptr);
	va_end(argptr);

	common->Error("File %s, line %i: %s", pi->parseFile, pi->lines, string);
}

void Com_ScriptWarning(const char* msg, ...)
{
	va_list argptr;
	char string[32000];

	va_start(argptr, msg);
	vsprintf(string, msg,argptr);
	va_end(argptr);

	common->Printf("File %s, line %i: %s", pi->parseFile, pi->lines, string);
}


/*
Calling this will make the next Com_Parse return
the current token instead of advancing the pointer
*/
void Com_UngetToken()
{
	if (pi->ungetToken)
	{
		Com_ScriptError("UngetToken called twice");
	}
	pi->ungetToken = true;
}


static const char* SkipWhitespace(const char* data, bool* hasNewLines)
{
	int c;
	while ((c = *data) <= ' ')
	{
		if (!c)
		{
			return NULL;
		}
		if (c == '\n')
		{
			pi->lines++;
			*hasNewLines = true;
		}
		data++;
	}

	return data;
}

/*
Parse a token out of a string
Will never return NULL, just empty strings.
An empty string will only be returned at end of file.

If "allowLineBreaks" is true then an empty
string will be returned if the next token is
a newline.
*/
static char* Com_ParseExt(const char** data_p, bool allowLineBreaks)
{
	if (!data_p)
	{
		common->FatalError("Com_ParseExt: NULL data_p");
	}

	const char* data = *data_p;
	int len = 0;
	pi->token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return pi->token;
	}

	int c = 0;
	bool hasNewLines = false;
	// skip any leading whitespace
	while (1)
	{
		// skip whitespace
		data = SkipWhitespace(data, &hasNewLines);
		if (!data)
		{
			*data_p = NULL;
			return pi->token;
		}
		if (hasNewLines && !allowLineBreaks)
		{
			*data_p = data;
			return pi->token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
			{
				data++;
			}
			continue;
		}

		// skip /* */ comments
		if (c == '/' && data[1] == '*')
		{
			while (*data && (*data != '*' || data[1] != '/'))
			{
				if (*data == '\n')
				{
					pi->lines++;
				}
				data++;
			}
			if (*data)
			{
				data += 2;
			}
			continue;
		}

		// a real token to parse
		break;
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if ((c == '\\') && (*data == '\"'))
			{
				// allow quoted strings to use \" to indicate the " character
				data++;
			}
			else if (c == '\"' || !c)
			{
				pi->token[len] = 0;
				*data_p = (char*)data;
				return pi->token;
			}
			else if (*data == '\n')
			{
				pi->lines++;
			}
			if (len < MAX_TOKEN_CHARS_Q3 - 1)
			{
				pi->token[len] = c;
				len++;
			}
		}
	}

	// check for a number
	// is this parsing of negative numbers going to cause expression problems
	if ((c >= '0' && c <= '9') || (c == '-' && data[1] >= '0' && data[1] <= '9') ||
		(c == '.' && data[1] >= '0' && data[1] <= '9'))
	{
		do
		{

			if (len < MAX_TOKEN_CHARS_Q3 - 1)
			{
				pi->token[len] = c;
				len++;
			}
			data++;

			c = *data;
		}
		while ((c >= '0' && c <= '9') || c == '.');

		// parse the exponent
		if (c == 'e' || c == 'E')
		{
			if (len < MAX_TOKEN_CHARS_Q3 - 1)
			{
				pi->token[len] = c;
				len++;
			}
			data++;
			c = *data;

			if (c == '-' || c == '+')
			{
				if (len < MAX_TOKEN_CHARS_Q3 - 1)
				{
					pi->token[len] = c;
					len++;
				}
				data++;
				c = *data;
			}

			do
			{
				if (len < MAX_TOKEN_CHARS_Q3 - 1)
				{
					pi->token[len] = c;
					len++;
				}
				data++;

				c = *data;
			}
			while (c >= '0' && c <= '9');
		}

		if (len == MAX_TOKEN_CHARS_Q3)
		{
			len = 0;
		}
		pi->token[len] = 0;

		*data_p = (char*)data;
		return pi->token;
	}

	// check for a regular word
	// we still allow forward and back slashes in name tokens for pathnames
	// and also colons for drive letters
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '/' || c == '\\')
	{
		do
		{
			if (len < MAX_TOKEN_CHARS_Q3 - 1)
			{
				pi->token[len] = c;
				len++;
			}
			data++;

			c = *data;
		}
		while ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' ||
			   (c >= '0' && c <= '9') || c == '/' || c == '\\' || c == ':' || c == '.');

		if (len == MAX_TOKEN_CHARS_Q3)
		{
			len = 0;
		}
		pi->token[len] = 0;

		*data_p = (char*)data;
		return pi->token;
	}

	// check for multi-character punctuation token
	for (const char** punc = punctuation; *punc; punc++)
	{
		int l;
		int j;

		l = String::Length(*punc);
		for (j = 0; j < l; j++)
		{
			if (data[j] != (*punc)[j])
			{
				break;
			}
		}
		if (j == l)
		{
			// a valid multi-character punctuation
			memcpy(pi->token, *punc, l);
			pi->token[l] = 0;
			data += l;
			*data_p = (char*)data;
			return pi->token;
		}
	}

	// single character punctuation
	pi->token[0] = *data;
	pi->token[1] = 0;
	data++;
	*data_p = (char*)data;

	return pi->token;
}

const char* Com_Parse(const char** data_p)
{
	if (pi->ungetToken)
	{
		pi->ungetToken = false;
		return pi->token;
	}
	return Com_ParseExt(data_p, true);
}

const char* Com_ParseOnLine(const char** data_p)
{
	if (pi->ungetToken)
	{
		pi->ungetToken = false;
		return pi->token;
	}
	return Com_ParseExt(data_p, false);
}

void Com_MatchToken(const char** buf_p, const char* match, bool warning)
{
	const char* token = Com_Parse(buf_p);
	if (String::Cmp(token, match))
	{
		if (warning)
		{
			Com_ScriptWarning("MatchToken: %s != %s", token, match);
		}
		else
		{
			Com_ScriptError("MatchToken: %s != %s", token, match);
		}
	}
}

float Com_ParseFloat(const char** buf_p)
{
	const char* token = Com_Parse(buf_p);
	if (!token[0])
	{
		return 0;
	}
	return String::Atof(token);
}

void Com_Parse1DMatrix(const char** buf_p, int x, float* m)
{
	Com_MatchToken(buf_p, "(");

	for (int i = 0; i < x; i++)
	{
		const char* token = Com_Parse(buf_p);
		m[i] = String::Atof(token);
	}

	Com_MatchToken(buf_p, ")");
}
