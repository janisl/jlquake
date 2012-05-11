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

script_t* LoadScriptMemory(const char* ptr, int length, const char* name)
{
	void* buffer = Mem_ClearedAlloc(sizeof(script_t) + length + 1);
	script_t* script = (script_t*)buffer;
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
	script->line = 1;
	script->lastline = 1;

	SetScriptPunctuations(script, NULL);

	Com_Memcpy(script->buffer, ptr, length);
	return script;
}

void FreeScript(script_t* script)
{
	if (script->punctuationtable)
	{
		Mem_Free(script->punctuationtable);
	}
	Mem_Free(script);
}

void ScriptError(script_t* script, const char* str, ...)
{
	if (script->flags & SCFL_NOERRORS)
	{
		return;
	}

	va_list ap;
	char text[1024];
	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	common->Printf(S_COLOR_RED "Error: file %s, line %d: %s\n", script->filename, script->line, text);
}

void ScriptWarning(script_t* script, const char* str, ...)
{
	if (script->flags & SCFL_NOWARNINGS)
	{
		return;
	}

	va_list ap;
	char text[1024];
	va_start(ap, str);
	Q_vsnprintf(text, sizeof(text), str, ap);
	va_end(ap);
	common->Printf(S_COLOR_YELLOW "Warning: file %s, line %d: %s\n", script->filename, script->line, text);
}

// Reads spaces, tabs, C-like comments etc.
// When a newline character is found the scripts line counter is increased.
bool PS_ReadWhiteSpace(script_t* script)
{
	while (1)
	{
		//skip white space
		while (*script->script_p <= ' ')
		{
			if (!*script->script_p)
			{
				return false;
			}
			if (*script->script_p == '\n')
			{
				script->line++;
			}
			script->script_p++;
		}
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
				}
				while (*script->script_p != '\n');
				script->line++;
				script->script_p++;
				if (!*script->script_p)
				{
					return false;
				}
				continue;
			}
			//comments /* */
			else if (*(script->script_p + 1) == '*')
			{
				script->script_p++;
				do
				{
					script->script_p++;
					if (!*script->script_p)
					{
						return false;
					}
					if (*script->script_p == '\n')
					{
						script->line++;
					}
				}
				while (!(*script->script_p == '*' && *(script->script_p + 1) == '/'));
				script->script_p++;
				if (!*script->script_p)
				{
					return false;
				}
				script->script_p++;
				if (!*script->script_p)
				{
					return false;
				}
				continue;
			}
		}
		break;
	}
	return true;
}

// Reads an escape character.
static bool PS_ReadEscapeCharacter(script_t* script, char* ch)
{
	//step over the leading '\\'
	script->script_p++;
	//determine the escape character
	int c;
	switch (*script->script_p)
	{
	case '\\': c = '\\'; break;
	case 'n': c = '\n'; break;
	case 'r': c = '\r'; break;
	case 't': c = '\t'; break;
	case 'v': c = '\v'; break;
	case 'b': c = '\b'; break;
	case 'f': c = '\f'; break;
	case 'a': c = '\a'; break;
	case '\'': c = '\''; break;
	case '\"': c = '\"'; break;
	case '\?': c = '\?'; break;
	case 'x':
	{
		script->script_p++;
		int val = 0;
		for (int i = 0;; i++, script->script_p++)
		{
			c = *script->script_p;
			if (c >= '0' && c <= '9')
			{
				c = c - '0';
			}
			else if (c >= 'A' && c <= 'Z')
			{
				c = c - 'A' + 10;
			}
			else if (c >= 'a' && c <= 'z')
			{
				c = c - 'a' + 10;
			}
			else
			{
				break;
			}
			val = (val << 4) + c;
		}
		script->script_p--;
		if (val > 0xFF)
		{
			ScriptWarning(script, "too large value in escape character");
			val = 0xFF;
		}
		c = val;
		break;
	}
	default:	//NOTE: decimal ASCII code, NOT octal
	{
		if (*script->script_p < '0' || *script->script_p > '9')
		{
			ScriptError(script, "unknown escape char");
		}
		int val = 0;
		for (int i = 0;; i++, script->script_p++)
		{
			c = *script->script_p;
			if (c >= '0' && c <= '9')
			{
				c = c - '0';
			}
			else
			{
				break;
			}
			val = val * 10 + c;
		}
		script->script_p--;
		if (val > 0xFF)
		{
			ScriptWarning(script, "too large value in escape character");
			val = 0xFF;
		}
		c = val;
		break;
	}
	}
	//step over the escape character or the last digit of the number
	script->script_p++;
	//store the escape character
	*ch = c;
	//succesfully read escape character
	return true;
}

// Reads C-like string. Escape characters are interpretted.
// Quotes are included with the string.
// Reads two strings with a white space between them as one string.
static bool PS_ReadString(script_t* script, token_t* token, int quote)
{
	if (quote == '\"')
	{
		token->type = TT_STRING;
	}
	else
	{
		token->type = TT_LITERAL;
	}

	int len = 0;
	//leading quote
	token->string[len++] = *script->script_p++;
	while (1)
	{
		//minus 2 because trailing double quote and zero have to be appended
		if (len >= MAX_TOKEN - 2)
		{
			ScriptError(script, "string longer than MAX_TOKEN = %d", MAX_TOKEN);
			return false;
		}
		//if there is an escape character and
		//if escape characters inside a string are allowed
		if (*script->script_p == '\\' && !(script->flags & SCFL_NOSTRINGESCAPECHARS))
		{
			if (!PS_ReadEscapeCharacter(script, &token->string[len]))
			{
				token->string[len] = 0;
				return false;
			}
			len++;
		}
		//if a trailing quote
		else if (*script->script_p == quote)
		{
			//step over the double quote
			script->script_p++;
			//if white spaces in a string are not allowed
			if (script->flags & SCFL_NOSTRINGWHITESPACES)
			{
				break;
			}
			char* tmpscript_p = script->script_p;
			int tmpline = script->line;
			//read unusefull stuff between possible two following strings
			if (!PS_ReadWhiteSpace(script))
			{
				script->script_p = tmpscript_p;
				script->line = tmpline;
				break;
			}
			//if there's no leading double qoute
			if (*script->script_p != quote)
			{
				script->script_p = tmpscript_p;
				script->line = tmpline;
				break;
			}
			//step over the new leading double quote
			script->script_p++;
		}
		else
		{
			if (*script->script_p == '\0')
			{
				token->string[len] = 0;
				ScriptError(script, "missing trailing quote");
				return false;
			}
			if (*script->script_p == '\n')
			{
				token->string[len] = 0;
				ScriptError(script, "newline inside string %s", token->string);
				return false;
			}
			token->string[len++] = *script->script_p++;
		}
	}
	//trailing quote
	token->string[len++] = quote;
	//end string with a zero
	token->string[len] = '\0';
	//the sub type is the length of the string
	token->subtype = len;
	return true;
}

static void NumberValue(const char* string, int subtype, unsigned int* intvalue,
	long double* floatvalue)
{
	*intvalue = 0;
	*floatvalue = 0;
	//floating point number
	if (subtype & TT_FLOAT)
	{
		unsigned int dotfound = 0;
		while (*string)
		{
			if (*string == '.')
			{
				if (dotfound)
				{
					return;
				}
				dotfound = 10;
				string++;
			}
			if (dotfound)
			{
				*floatvalue = *floatvalue + (long double)(*string - '0') /
							  (long double)dotfound;
				dotfound *= 10;
			}
			else
			{
				*floatvalue = *floatvalue * 10.0 + (long double)(*string - '0');
			}
			string++;
		}
		*intvalue = (unsigned int)*floatvalue;
	}
	else if (subtype & TT_DECIMAL)
	{
		while (*string)
		{
			*intvalue = *intvalue * 10 + (*string++ - '0');
		}
		*floatvalue = *intvalue;
	}
	else if (subtype & TT_HEX)
	{
		//step over the leading 0x or 0X
		string += 2;
		while (*string)
		{
			*intvalue <<= 4;
			if (*string >= 'a' && *string <= 'f')
			{
				*intvalue += *string - 'a' + 10;
			}
			else if (*string >= 'A' && *string <= 'F')
			{
				*intvalue += *string - 'A' + 10;
			}
			else
			{
				*intvalue += *string - '0';
			}
			string++;
		}
		*floatvalue = *intvalue;
	}
	else if (subtype & TT_OCTAL)
	{
		//step over the first zero
		string += 1;
		while (*string)
		{
			*intvalue = (*intvalue << 3) + (*string++ - '0');
		}
		*floatvalue = *intvalue;
	}
	else if (subtype & TT_BINARY)
	{
		//step over the leading 0b or 0B
		string += 2;
		while (*string)
		{
			*intvalue = (*intvalue << 1) + (*string++ - '0');
		}
		*floatvalue = *intvalue;
	}
}

static bool PS_ReadNumber(script_t* script, token_t* token)
{
	int len = 0;

	token->type = TT_NUMBER;
	//check for a hexadecimal number
	if (*script->script_p == '0' &&
		(*(script->script_p + 1) == 'x' ||
		 *(script->script_p + 1) == 'X'))
	{
		token->string[len++] = *script->script_p++;
		token->string[len++] = *script->script_p++;
		char c = *script->script_p;
		//hexadecimal
		while ((c >= '0' && c <= '9') ||
			   (c >= 'a' && c <= 'f') ||
			   (c >= 'A' && c <= 'A'))
		{
			token->string[len++] = *script->script_p++;
			if (len >= MAX_TOKEN)
			{
				ScriptError(script, "hexadecimal number longer than MAX_TOKEN = %d", MAX_TOKEN);
				return false;
			}
			c = *script->script_p;
		}
		token->subtype |= TT_HEX;
	}
	//check for a binary number
	else if (*script->script_p == '0' &&
			 (*(script->script_p + 1) == 'b' ||
			  *(script->script_p + 1) == 'B'))
	{
		token->string[len++] = *script->script_p++;
		token->string[len++] = *script->script_p++;
		char c = *script->script_p;
		//binary
		while (c == '0' || c == '1')
		{
			token->string[len++] = *script->script_p++;
			if (len >= MAX_TOKEN)
			{
				ScriptError(script, "binary number longer than MAX_TOKEN = %d", MAX_TOKEN);
				return 0;
			}
			c = *script->script_p;
		}
		token->subtype |= TT_BINARY;
	}
	else//decimal or octal integer or floating point number
	{
		bool octal = false;
		int dot = false;
		if (*script->script_p == '0')
		{
			octal = true;
		}
		while (1)
		{
			char c = *script->script_p;
			if (c == '.')
			{
				dot = true;
			}
			else if (c == '8' || c == '9')
			{
				octal = false;
			}
			else if (c < '0' || c > '9')
			{
				break;
			}
			token->string[len++] = *script->script_p++;
			if (len >= MAX_TOKEN - 1)
			{
				ScriptError(script, "number longer than MAX_TOKEN = %d", MAX_TOKEN);
				return false;
			}
		}
		if (octal)
		{
			token->subtype |= TT_OCTAL;
		}
		else
		{
			token->subtype |= TT_DECIMAL;
		}
		if (dot)
		{
			token->subtype |= TT_FLOAT;
		}
	}
	for (int i = 0; i < 2; i++)
	{
		char c = *script->script_p;
		//check for a LONG number
		if ((c == 'l' || c == 'L') &&
			!(token->subtype & TT_LONG))
		{
			script->script_p++;
			token->subtype |= TT_LONG;
		}
		//check for an UNSIGNED number
		else if ((c == 'u' || c == 'U') &&
				 !(token->subtype & (TT_UNSIGNED | TT_FLOAT)))
		{
			script->script_p++;
			token->subtype |= TT_UNSIGNED;
		}
	}
	token->string[len] = '\0';
	NumberValue(token->string, token->subtype, &token->intvalue, &token->floatvalue);
	if (!(token->subtype & TT_FLOAT))
	{
		token->subtype |= TT_INTEGER;
	}
	return true;
}

static bool PS_ReadName(script_t* script, token_t* token)
{
	token->type = TT_NAME;
	int len = 0;
	char c;
	do
	{
		token->string[len++] = *script->script_p++;
		if (len >= MAX_TOKEN)
		{
			ScriptError(script, "name longer than MAX_TOKEN = %d", MAX_TOKEN);
			return false;
		}
		c = *script->script_p;
	}
	while ((c >= 'a' && c <= 'z') ||
		   (c >= 'A' && c <= 'Z') ||
		   (c >= '0' && c <= '9') ||
		   c == '_');
	token->string[len] = '\0';
	//the sub type is the length of the name
	token->subtype = len;
	return true;
}

static bool PS_ReadPunctuation(script_t* script, token_t* token)
{
	for (punctuation_t* punc = script->punctuationtable[(unsigned int)*script->script_p]; punc; punc = punc->next)
	{
		const char* p = punc->p;
		int len = String::Length(p);
		//if the script contains at least as much characters as the punctuation
		if (script->script_p + len <= script->end_p)
		{
			//if the script contains the punctuation
			if (!String::NCmp(script->script_p, p, len))
			{
				String::NCpy(token->string, p, MAX_TOKEN);
				script->script_p += len;
				token->type = TT_PUNCTUATION;
				//sub type is the number of the punctuation
				token->subtype = punc->n;
				return true;
			}
		}
	}
	return false;
}

static bool PS_ReadPrimitive(script_t* script, token_t* token)
{
	int len = 0;
	while (*script->script_p > ' ' && *script->script_p != ';')
	{
		if (len >= MAX_TOKEN)
		{
			ScriptError(script, "primitive token longer than MAX_TOKEN = %d", MAX_TOKEN);
			return false;
		}
		token->string[len++] = *script->script_p++;
	}
	token->string[len] = 0;
	//copy the token into the script structure
	Com_Memcpy(&script->token, token, sizeof(token_t));
	//primitive reading successfull
	return true;
}

bool PS_ReadToken(script_t* script, token_t* token)
{
	//if there is a token available (from UnreadToken)
	if (script->tokenavailable)
	{
		script->tokenavailable = false;
		Com_Memcpy(token, &script->token, sizeof(token_t));
		return true;
	}
	//save script pointer
	script->lastscript_p = script->script_p;
	//save line counter
	script->lastline = script->line;
	//clear the token stuff
	Com_Memset(token, 0, sizeof(token_t));
	//start of the white space
	script->whitespace_p = script->script_p;
	token->whitespace_p = script->script_p;
	//read unusefull stuff
	if (!PS_ReadWhiteSpace(script))
	{
		return false;
	}
	//end of the white space
	script->endwhitespace_p = script->script_p;
	token->endwhitespace_p = script->script_p;
	//line the token is on
	token->line = script->line;
	//number of lines crossed before token
	token->linescrossed = script->line - script->lastline;
	//if there is a leading double quote
	if (*script->script_p == '\"')
	{
		if (!PS_ReadString(script, token, '\"'))
		{
			return false;
		}
	}
	//if an literal
	else if (*script->script_p == '\'')
	{
		if (!PS_ReadString(script, token, '\''))
		{
			return false;
		}
	}
	//if there is a number
	else if ((*script->script_p >= '0' && *script->script_p <= '9') ||
			 (*script->script_p == '.' &&
			  (*(script->script_p + 1) >= '0' && *(script->script_p + 1) <= '9')))
	{
		if (!PS_ReadNumber(script, token))
		{
			return false;
		}
	}
	//if this is a primitive script
	else if (script->flags & SCFL_PRIMITIVE)
	{
		return PS_ReadPrimitive(script, token);
	}
	//if there is a name
	else if ((*script->script_p >= 'a' && *script->script_p <= 'z') ||
			 (*script->script_p >= 'A' && *script->script_p <= 'Z') ||
			 *script->script_p == '_')
	{
		if (!PS_ReadName(script, token))
		{
			return false;
		}
	}
	//check for punctuations
	else if (!PS_ReadPunctuation(script, token))
	{
		ScriptError(script, "can't read token");
		return false;
	}
	//copy the token into the script structure
	Com_Memcpy(&script->token, token, sizeof(token_t));
	//succesfully read a token
	return true;
}

// returns true if at the end of the script
bool EndOfScript(script_t* script)
{
	return script->script_p >= script->end_p;
}
