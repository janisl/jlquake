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

/*****************************************************************************
 * name:		l_script.h
 *
 * desc:		lexicographical parser
 *
 * $Archive: /source/code/botlib/l_script.h $
 *
 *****************************************************************************/

//script flags
#define SCFL_NOERRORS               0x0001
#define SCFL_NOWARNINGS             0x0002
#define SCFL_NOSTRINGWHITESPACES    0x0004
#define SCFL_NOSTRINGESCAPECHARS    0x0008
#define SCFL_PRIMITIVE              0x0010
#define SCFL_NOBINARYNUMBERS        0x0020
#define SCFL_NONUMBERVALUES     0x0040

//string sub type
//---------------
//		the length of the string
//literal sub type
//----------------
//		the ASCII code of the literal
//number sub type
//---------------
#define TT_DECIMAL                  0x0008	// decimal number
#define TT_HEX                          0x0100	// hexadecimal number
#define TT_OCTAL                        0x0200	// octal number
#define TT_BINARY                       0x0400	// binary number
#define TT_FLOAT                        0x0800	// floating point number
#define TT_INTEGER                  0x1000	// integer number
#define TT_LONG                     0x2000	// long number
#define TT_UNSIGNED                 0x4000	// unsigned number
//name sub type
//-------------
//		the length of the name

//read a token from the script
int PS_ReadToken(script_t* script, token_t* token);
//expect a certain token
int PS_ExpectTokenString(script_t* script, char* string);
//expect a certain token type
int PS_ExpectTokenType(script_t* script, int type, int subtype, token_t* token);
//expect a token
int PS_ExpectAnyToken(script_t* script, token_t* token);
//returns true when the token is available
int PS_CheckTokenString(script_t* script, char* string);
//returns true an reads the token when a token with the given type is available
int PS_CheckTokenType(script_t* script, int type, int subtype, token_t* token);
//skip tokens until the given token string is read
int PS_SkipUntilString(script_t* script, char* string);
//unread the last token read from the script
void PS_UnreadLastToken(script_t* script);
//unread the given token
void PS_UnreadToken(script_t* script, token_t* token);
//returns the next character of the read white space, returns NULL if none
char PS_NextWhiteSpaceChar(script_t* script);
//remove any leading and trailing double quotes from the token
void StripDoubleQuotes(char* string);
//remove any leading and trailing single quotes from the token
void StripSingleQuotes(char* string);
//read a possible signed integer
signed long int ReadSignedInt(script_t* script);
//read a possible signed floating point number
long double ReadSignedFloat(script_t* script);
//set script flags
void SetScriptFlags(script_t* script, int flags);
//get script flags
int GetScriptFlags(script_t* script);
//reset a script
void ResetScript(script_t* script);
//returns true if at the end of the script
int EndOfScript(script_t* script);
//returns a pointer to the punctuation with the given number
const char* PunctuationFromNum(script_t* script, int num);
//print a script error with filename and line number
void ScriptError(script_t* script, const char* str, ...);
//print a script warning with filename and line number
void ScriptWarning(script_t* script, const char* str, ...);
