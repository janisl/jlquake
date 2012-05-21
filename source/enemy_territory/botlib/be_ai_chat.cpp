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
 * name:		be_ai_chat.c
 *
 * desc:		bot chat AI
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
//#include "../server/server.h"
#include "l_memory.h"
#include "l_utils.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "../game/be_ea.h"
#include "../game/be_ai_chat.h"


//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int IsWhiteSpace(char c)
{
	if ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '(' || c == ')' ||
		c == '?' || c == '\'' ||
		c == ':' || c == ',' ||
		c == '['  || c == ']' ||
		c == '-' || c == '_' ||
		c == '+' || c == '=')
	{
		return qfalse;
	}
	return qtrue;
}	//end of the function IsWhiteSpace
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotRemoveTildes(char* message)
{
	int i;

	//remove all tildes from the chat message
	for (i = 0; message[i]; i++)
	{
		if (message[i] == '~')
		{
			memmove(&message[i], &message[i + 1], String::Length(&message[i + 1]) + 1);
		}	//end if
	}	//end for
}	//end of the function BotRemoveTildes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void UnifyWhiteSpaces(char* string)
{
	char* ptr, * oldptr;

	for (ptr = oldptr = string; *ptr; oldptr = ptr)
	{
		while (*ptr && IsWhiteSpace(*ptr))
			ptr++;
		if (ptr > oldptr)
		{
			//if not at the start and not at the end of the string
			//write only one space
			if (oldptr > string && *ptr)
			{
				*oldptr++ = ' ';
			}
			//remove all other white spaces
			if (ptr > oldptr)
			{
				memmove(oldptr, ptr, String::Length(ptr) + 1);
			}
		}	//end if
		while (*ptr && !IsWhiteSpace(*ptr))
			ptr++;
	}	//end while
}	//end of the function UnifyWhiteSpaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int StringContains(char* str1, char* str2, int casesensitive)
{
	int len, i, j, index;

	if (str1 == NULL || str2 == NULL)
	{
		return -1;
	}

	len = String::Length(str1) - String::Length(str2);
	index = 0;
	for (i = 0; i <= len; i++, str1++, index++)
	{
		for (j = 0; str2[j]; j++)
		{
			if (casesensitive)
			{
				if (str1[j] != str2[j])
				{
					break;
				}
			}	//end if
			else
			{
				if (String::ToUpper(str1[j]) != String::ToUpper(str2[j]))
				{
					break;
				}
			}	//end else
		}	//end for
		if (!str2[j])
		{
			return index;
		}
	}	//end for
	return -1;
}	//end of the function StringContains
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char* StringContainsWord(char* str1, char* str2, int casesensitive)
{
	int len, i, j;

	len = String::Length(str1) - String::Length(str2);
	for (i = 0; i <= len; i++, str1++)
	{
		//if not at the start of the string
		if (i)
		{
			//skip to the start of the next word
			while (*str1 && *str1 != ' ')
				str1++;
			if (!*str1)
			{
				break;
			}
			str1++;
		}	//end for
			//compare the word
		for (j = 0; str2[j]; j++)
		{
			if (casesensitive)
			{
				if (str1[j] != str2[j])
				{
					break;
				}
			}	//end if
			else
			{
				if (String::ToUpper(str1[j]) != String::ToUpper(str2[j]))
				{
					break;
				}
			}	//end else
		}	//end for
			//if there was a word match
		if (!str2[j])
		{
			//if the first string has an end of word
			if (!str1[j] || str1[j] == ' ')
			{
				return str1;
			}
		}	//end if
	}	//end for
	return NULL;
}	//end of the function StringContainsWord
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void StringReplaceWords(char* string, char* synonym, char* replacement)
{
	char* str, * str2;

	//find the synonym in the string
	str = StringContainsWord(string, synonym, qfalse);
	//if the synonym occured in the string
	while (str)
	{
		//if the synonym isn't part of the replacement which is already in the string
		//usefull for abreviations
		str2 = StringContainsWord(string, replacement, qfalse);
		while (str2)
		{
			if (str2 <= str && str < str2 + String::Length(replacement))
			{
				break;
			}
			str2 = StringContainsWord(str2 + 1, replacement, qfalse);
		}	//end while
		if (!str2)
		{
			memmove(str + String::Length(replacement), str + String::Length(synonym), String::Length(str + String::Length(synonym)) + 1);
			//append the synonum replacement
			memcpy(str, replacement, String::Length(replacement));
		}	//end if
			//find the next synonym in the string
		str = StringContainsWord(str + String::Length(replacement), synonym, qfalse);
	}	//end if
}	//end of the function StringReplaceWords
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_synonymlist_t* BotLoadSynonyms(const char* filename)
{
	int pass, size, contextlevel, numsynonyms;
	unsigned long int context, contextstack[32];
	char* ptr = NULL;
	source_t* source;
	token_t token;
	bot_synonymlist_t* synlist, * lastsyn, * syn;
	bot_synonym_t* synonym, * lastsynonym;

	size = 0;
	synlist = NULL;	//make compiler happy
	syn = NULL;	//make compiler happy
	synonym = NULL;	//make compiler happy
	//the synonyms are parsed in two phases
	for (pass = 0; pass < 2; pass++)
	{
		//
		if (pass && size)
		{
			ptr = (char*)GetClearedHunkMemory(size);
		}
		//
		source = LoadSourceFile(filename);
		if (!source)
		{
			BotImport_Print(PRT_ERROR, "counldn't load %s\n", filename);
			return NULL;
		}	//end if
			//
		context = 0;
		contextlevel = 0;
		synlist = NULL;	//list synonyms
		lastsyn = NULL;	//last synonym in the list
		//
		while (PC_ReadToken(source, &token))
		{
			if (token.type == TT_NUMBER)
			{
				context |= token.intvalue;
				contextstack[contextlevel] = token.intvalue;
				contextlevel++;
				if (contextlevel >= 32)
				{
					SourceError(source, "more than 32 context levels");
					FreeSource(source);
					return NULL;
				}	//end if
				if (!PC_ExpectTokenString(source, "{"))
				{
					FreeSource(source);
					return NULL;
				}	//end if
			}	//end if
			else if (token.type == TT_PUNCTUATION)
			{
				if (!String::Cmp(token.string, "}"))
				{
					contextlevel--;
					if (contextlevel < 0)
					{
						SourceError(source, "too many }");
						FreeSource(source);
						return NULL;
					}	//end if
					context &= ~contextstack[contextlevel];
				}	//end if
				else if (!String::Cmp(token.string, "["))
				{
					size += sizeof(bot_synonymlist_t);
					if (pass)
					{
						syn = (bot_synonymlist_t*)ptr;
						ptr += sizeof(bot_synonymlist_t);
						syn->context = context;
						syn->firstsynonym = NULL;
						syn->next = NULL;
						if (lastsyn)
						{
							lastsyn->next = syn;
						}
						else
						{
							synlist = syn;
						}
						lastsyn = syn;
					}	//end if
					numsynonyms = 0;
					lastsynonym = NULL;
					while (1)
					{
						if (!PC_ExpectTokenString(source, "(") ||
							!PC_ExpectTokenType(source, TT_STRING, 0, &token))
						{
							FreeSource(source);
							return NULL;
						}	//end if
						StripDoubleQuotes(token.string);
						if (String::Length(token.string) <= 0)
						{
							SourceError(source, "empty string");
							FreeSource(source);
							return NULL;
						}	//end if
						size += sizeof(bot_synonym_t) + String::Length(token.string) + 1;
						if (pass)
						{
							synonym = (bot_synonym_t*)ptr;
							ptr += sizeof(bot_synonym_t);
							synonym->string = ptr;
							ptr += String::Length(token.string) + 1;
							String::Cpy(synonym->string, token.string);
							//
							if (lastsynonym)
							{
								lastsynonym->next = synonym;
							}
							else
							{
								syn->firstsynonym = synonym;
							}
							lastsynonym = synonym;
						}	//end if
						numsynonyms++;
						if (!PC_ExpectTokenString(source, ",") ||
							!PC_ExpectTokenType(source, TT_NUMBER, 0, &token) ||
							!PC_ExpectTokenString(source, ")"))
						{
							FreeSource(source);
							return NULL;
						}	//end if
						if (pass)
						{
							synonym->weight = token.floatvalue;
							syn->totalweight += synonym->weight;
						}	//end if
						if (PC_CheckTokenString(source, "]"))
						{
							break;
						}
						if (!PC_ExpectTokenString(source, ","))
						{
							FreeSource(source);
							return NULL;
						}	//end if
					}	//end while
					if (numsynonyms < 2)
					{
						SourceError(source, "synonym must have at least two entries\n");
						FreeSource(source);
						return NULL;
					}	//end if
				}	//end else
				else
				{
					SourceError(source, "unexpected %s", token.string);
					FreeSource(source);
					return NULL;
				}	//end if
			}	//end else if
		}	//end while
			//
		FreeSource(source);
		//
		if (contextlevel > 0)
		{
			SourceError(source, "missing }");
			return NULL;
		}	//end if
	}	//end for
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);
	return synlist;
}	//end of the function BotLoadSynonyms
//===========================================================================
// replace all the synonyms in the string
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotReplaceSynonyms(char* string, unsigned long int context)
{
	bot_synonymlist_t* syn;
	bot_synonym_t* synonym;

	for (syn = synonyms; syn; syn = syn->next)
	{
		if (!(syn->context & context))
		{
			continue;
		}
		for (synonym = syn->firstsynonym->next; synonym; synonym = synonym->next)
		{
			StringReplaceWords(string, synonym->string, syn->firstsynonym->string);
		}	//end for
	}	//end for
}	//end of the function BotReplaceSynonyms
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotReplaceWeightedSynonyms(char* string, unsigned long int context)
{
	bot_synonymlist_t* syn;
	bot_synonym_t* synonym, * replacement;
	float weight, curweight;

	for (syn = synonyms; syn; syn = syn->next)
	{
		if (!(syn->context & context))
		{
			continue;
		}
		//choose a weighted random replacement synonym
		weight = random() * syn->totalweight;
		if (!weight)
		{
			continue;
		}
		curweight = 0;
		for (replacement = syn->firstsynonym; replacement; replacement = replacement->next)
		{
			curweight += replacement->weight;
			if (weight < curweight)
			{
				break;
			}
		}	//end for
		if (!replacement)
		{
			continue;
		}
		//replace all synonyms with the replacement
		for (synonym = syn->firstsynonym; synonym; synonym = synonym->next)
		{
			if (synonym == replacement)
			{
				continue;
			}
			StringReplaceWords(string, synonym->string, replacement->string);
		}	//end for
	}	//end for
}	//end of the function BotReplaceWeightedSynonyms
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotReplaceReplySynonyms(char* string, unsigned long int context)
{
	char* str1, * str2, * replacement;
	bot_synonymlist_t* syn;
	bot_synonym_t* synonym;

	for (str1 = string; *str1; )
	{
		//go to the start of the next word
		while (*str1 && *str1 <= ' ')
			str1++;
		if (!*str1)
		{
			break;
		}
		//
		for (syn = synonyms; syn; syn = syn->next)
		{
			if (!(syn->context & context))
			{
				continue;
			}
			for (synonym = syn->firstsynonym->next; synonym; synonym = synonym->next)
			{
				str2 = synonym->string;
				//if the synonym is not at the front of the string continue
				str2 = StringContainsWord(str1, synonym->string, qfalse);
				if (!str2 || str2 != str1)
				{
					continue;
				}
				//
				replacement = syn->firstsynonym->string;
				//if the replacement IS in front of the string continue
				str2 = StringContainsWord(str1, replacement, qfalse);
				if (str2 && str2 == str1)
				{
					continue;
				}
				//
				memmove(str1 + String::Length(replacement), str1 + String::Length(synonym->string),
					String::Length(str1 + String::Length(synonym->string)) + 1);
				//append the synonum replacement
				memcpy(str1, replacement, String::Length(replacement));
				//
				break;
			}	//end for
				//if a synonym has been replaced
			if (synonym)
			{
				break;
			}
		}	//end for
			//skip over this word
		while (*str1 && *str1 > ' ')
			str1++;
		if (!*str1)
		{
			break;
		}
	}	//end while
}	//end of the function BotReplaceReplySynonyms
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotLoadChatMessage(source_t* source, char* chatmessagestring)
{
	char* ptr;
	token_t token;

	ptr = chatmessagestring;
	*ptr = 0;
	//
	while (1)
	{
		if (!PC_ExpectAnyToken(source, &token))
		{
			return qfalse;
		}
		//fixed string
		if (token.type == TT_STRING)
		{
			StripDoubleQuotes(token.string);
			if (String::Length(ptr) + String::Length(token.string) + 1 > MAX_MESSAGE_SIZE_WOLF)
			{
				SourceError(source, "chat message too long\n");
				return qfalse;
			}	//end if
			strcat(ptr, token.string);
		}	//end else if
			//variable string
		else if (token.type == TT_NUMBER && (token.subtype & TT_INTEGER))
		{
			if (String::Length(ptr) + 7 > MAX_MESSAGE_SIZE_WOLF)
			{
				SourceError(source, "chat message too long\n");
				return qfalse;
			}	//end if
			sprintf(&ptr[String::Length(ptr)], "%cv%d%c", ESCAPE_CHAR, token.intvalue, ESCAPE_CHAR);
		}	//end if
			//random string
		else if (token.type == TT_NAME)
		{
			if (String::Length(ptr) + 7 > MAX_MESSAGE_SIZE_WOLF)
			{
				SourceError(source, "chat message too long\n");
				return qfalse;
			}	//end if
			sprintf(&ptr[String::Length(ptr)], "%cr%s%c", ESCAPE_CHAR, token.string, ESCAPE_CHAR);
		}	//end else if
		else
		{
			SourceError(source, "unknown message component %s\n", token.string);
			return qfalse;
		}	//end else
		if (PC_CheckTokenString(source, ";"))
		{
			break;
		}
		if (!PC_ExpectTokenString(source, ","))
		{
			return qfalse;
		}
	}	//end while
		//
	return qtrue;
}	//end of the function BotLoadChatMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_randomlist_t* BotLoadRandomStrings(const char* filename)
{
	int pass, size;
	char* ptr = NULL, chatmessagestring[MAX_MESSAGE_SIZE_WOLF];
	source_t* source;
	token_t token;
	bot_randomlist_t* randomlist, * lastrandom, * random;
	bot_randomstring_t* randomstring;

#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif	//DEBUG

	size = 0;
	randomlist = NULL;
	random = NULL;
	//the synonyms are parsed in two phases
	for (pass = 0; pass < 2; pass++)
	{
		//
		if (pass && size)
		{
			ptr = (char*)GetClearedHunkMemory(size);
		}
		//
		source = LoadSourceFile(filename);
		if (!source)
		{
			BotImport_Print(PRT_ERROR, "counldn't load %s\n", filename);
			return NULL;
		}	//end if
			//
		randomlist = NULL;	//list
		lastrandom = NULL;	//last
		//
		while (PC_ReadToken(source, &token))
		{
			if (token.type != TT_NAME)
			{
				SourceError(source, "unknown random %s", token.string);
				FreeSource(source);
				return NULL;
			}	//end if
			size += sizeof(bot_randomlist_t) + String::Length(token.string) + 1;
			if (pass)
			{
				random = (bot_randomlist_t*)ptr;
				ptr += sizeof(bot_randomlist_t);
				random->string = ptr;
				ptr += String::Length(token.string) + 1;
				String::Cpy(random->string, token.string);
				random->firstrandomstring = NULL;
				random->numstrings = 0;
				//
				if (lastrandom)
				{
					lastrandom->next = random;
				}
				else
				{
					randomlist = random;
				}
				lastrandom = random;
			}	//end if
			if (!PC_ExpectTokenString(source, "=") ||
				!PC_ExpectTokenString(source, "{"))
			{
				FreeSource(source);
				return NULL;
			}	//end if
			while (!PC_CheckTokenString(source, "}"))
			{
				if (!BotLoadChatMessage(source, chatmessagestring))
				{
					FreeSource(source);
					return NULL;
				}	//end if
				size += sizeof(bot_randomstring_t) + String::Length(chatmessagestring) + 1;
				if (pass)
				{
					randomstring = (bot_randomstring_t*)ptr;
					ptr += sizeof(bot_randomstring_t);
					randomstring->string = ptr;
					ptr += String::Length(chatmessagestring) + 1;
					String::Cpy(randomstring->string, chatmessagestring);
					//
					random->numstrings++;
					randomstring->next = random->firstrandomstring;
					random->firstrandomstring = randomstring;
				}	//end if
			}	//end while
		}	//end while
			//free the source after one pass
		FreeSource(source);
	}	//end for
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);
	//
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "random strings %d msec\n", Sys_Milliseconds() - starttime);
#endif	//DEBUG
		//
	return randomlist;
}	//end of the function BotLoadRandomStrings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char* RandomString(char* name)
{
	bot_randomlist_t* random;
	bot_randomstring_t* rs;
	int i;

	for (random = randomstrings; random; random = random->next)
	{
		if (!String::Cmp(random->string, name))
		{
			i = rand() % random->numstrings;
			for (rs = random->firstrandomstring; rs; rs = rs->next)
			{
				if (--i < 0)
				{
					break;
				}
			}	//end for
			if (rs)
			{
				return rs->string;
			}	//end if
		}	//end for
	}	//end for
	return NULL;
}	//end of the function RandomString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeMatchPieces(bot_matchpiece_t* matchpieces)
{
	bot_matchpiece_t* mp, * nextmp;
	bot_matchstring_t* ms, * nextms;

	for (mp = matchpieces; mp; mp = nextmp)
	{
		nextmp = mp->next;
		if (mp->type == MT_STRING)
		{
			for (ms = mp->firststring; ms; ms = nextms)
			{
				nextms = ms->next;
				FreeMemory(ms);
			}	//end for
		}	//end if
		FreeMemory(mp);
	}	//end for
}	//end of the function BotFreeMatchPieces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_matchpiece_t* BotLoadMatchPieces(source_t* source, const char* endtoken)
{
	int lastwasvariable, emptystring;
	token_t token;
	bot_matchpiece_t* matchpiece, * firstpiece, * lastpiece;
	bot_matchstring_t* matchstring, * lastmatchstring;

	firstpiece = NULL;
	lastpiece = NULL;
	//
	lastwasvariable = qfalse;
	//
	while (PC_ReadToken(source, &token))
	{
		if (token.type == TT_NUMBER && (token.subtype & TT_INTEGER))
		{
			if (token.intvalue < 0 || token.intvalue >= MAX_MATCHVARIABLES)
			{
				SourceError(source, "can't have more than %d match variables\n", MAX_MATCHVARIABLES);
				FreeSource(source);
				BotFreeMatchPieces(firstpiece);
				return NULL;
			}	//end if
			if (lastwasvariable)
			{
				SourceError(source, "not allowed to have adjacent variables\n");
				FreeSource(source);
				BotFreeMatchPieces(firstpiece);
				return NULL;
			}	//end if
			lastwasvariable = qtrue;
			//
			matchpiece = (bot_matchpiece_t*)GetClearedHunkMemory(sizeof(bot_matchpiece_t));
			matchpiece->type = MT_VARIABLE;
			matchpiece->variable = token.intvalue;
			matchpiece->next = NULL;
			if (lastpiece)
			{
				lastpiece->next = matchpiece;
			}
			else
			{
				firstpiece = matchpiece;
			}
			lastpiece = matchpiece;
		}	//end if
		else if (token.type == TT_STRING)
		{
			//
			matchpiece = (bot_matchpiece_t*)GetClearedHunkMemory(sizeof(bot_matchpiece_t));
			matchpiece->firststring = NULL;
			matchpiece->type = MT_STRING;
			matchpiece->variable = 0;
			matchpiece->next = NULL;
			if (lastpiece)
			{
				lastpiece->next = matchpiece;
			}
			else
			{
				firstpiece = matchpiece;
			}
			lastpiece = matchpiece;
			//
			lastmatchstring = NULL;
			emptystring = qfalse;
			//
			do
			{
				if (matchpiece->firststring)
				{
					if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
					{
						FreeSource(source);
						BotFreeMatchPieces(firstpiece);
						return NULL;
					}	//end if
				}	//end if
				StripDoubleQuotes(token.string);
				matchstring = (bot_matchstring_t*)GetClearedHunkMemory(sizeof(bot_matchstring_t) + String::Length(token.string) + 1);
				matchstring->string = (char*)matchstring + sizeof(bot_matchstring_t);
				String::Cpy(matchstring->string, token.string);
				if (!String::Length(token.string))
				{
					emptystring = qtrue;
				}
				matchstring->next = NULL;
				if (lastmatchstring)
				{
					lastmatchstring->next = matchstring;
				}
				else
				{
					matchpiece->firststring = matchstring;
				}
				lastmatchstring = matchstring;
			}
			while (PC_CheckTokenString(source, "|"));
			//if there was no empty string found
			if (!emptystring)
			{
				lastwasvariable = qfalse;
			}
		}	//end if
		else
		{
			SourceError(source, "invalid token %s\n", token.string);
			FreeSource(source);
			BotFreeMatchPieces(firstpiece);
			return NULL;
		}	//end else
		if (PC_CheckTokenString(source, endtoken))
		{
			break;
		}
		if (!PC_ExpectTokenString(source, ","))
		{
			FreeSource(source);
			BotFreeMatchPieces(firstpiece);
			return NULL;
		}	//end if
	}	//end while
	return firstpiece;
}	//end of the function BotLoadMatchPieces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeMatchTemplates(bot_matchtemplate_t* mt)
{
	bot_matchtemplate_t* nextmt;

	for (; mt; mt = nextmt)
	{
		nextmt = mt->next;
		BotFreeMatchPieces(mt->first);
		FreeMemory(mt);
	}	//end for
}	//end of the function BotFreeMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_matchtemplate_t* BotLoadMatchTemplates(const char* matchfile)
{
	source_t* source;
	token_t token;
	bot_matchtemplate_t* matchtemplate, * matches, * lastmatch;
	unsigned long int context;

	source = LoadSourceFile(matchfile);
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", matchfile);
		return NULL;
	}	//end if
		//
	matches = NULL;	//list with matches
	lastmatch = NULL;	//last match in the list

	while (PC_ReadToken(source, &token))
	{
		if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER))
		{
			SourceError(source, "expected integer, found %s\n", token.string);
			BotFreeMatchTemplates(matches);
			FreeSource(source);
			return NULL;
		}	//end if
			//the context
		context = token.intvalue;
		//
		if (!PC_ExpectTokenString(source, "{"))
		{
			BotFreeMatchTemplates(matches);
			FreeSource(source);
			return NULL;
		}	//end if
			//
		while (PC_ReadToken(source, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}
			//
			PC_UnreadLastToken(source);
			//
			matchtemplate = (bot_matchtemplate_t*)GetClearedHunkMemory(sizeof(bot_matchtemplate_t));
			matchtemplate->context = context;
			matchtemplate->next = NULL;
			//add the match template to the list
			if (lastmatch)
			{
				lastmatch->next = matchtemplate;
			}
			else
			{
				matches = matchtemplate;
			}
			lastmatch = matchtemplate;
			//load the match template
			matchtemplate->first = BotLoadMatchPieces(source, "=");
			if (!matchtemplate->first)
			{
				BotFreeMatchTemplates(matches);
				return NULL;
			}	//end if
				//read the match type
			if (!PC_ExpectTokenString(source, "(") ||
				!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return NULL;
			}	//end if
			matchtemplate->type = token.intvalue;
			//read the match subtype
			if (!PC_ExpectTokenString(source, ",") ||
				!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return NULL;
			}	//end if
			matchtemplate->subtype = token.intvalue;
			//read trailing punctuations
			if (!PC_ExpectTokenString(source, ")") ||
				!PC_ExpectTokenString(source, ";"))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return NULL;
			}	//end if
		}	//end while
	}	//end while
		//free the source
	FreeSource(source);
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", matchfile);
	//
	return matches;
}	//end of the function BotLoadMatchTemplates
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int StringsMatch(bot_matchpiece_t* pieces, bot_match_wolf_t* match)
{
	int lastvariable, index;
	char* strptr, * newstrptr;
	bot_matchpiece_t* mp;
	bot_matchstring_t* ms;

	//no last variable
	lastvariable = -1;
	//pointer to the string to compare the match string with
	strptr = match->string;
	//Log_Write("match: %s", strptr);
	//compare the string with the current match string
	for (mp = pieces; mp; mp = mp->next)
	{
		//if it is a piece of string
		if (mp->type == MT_STRING)
		{
			newstrptr = NULL;
			for (ms = mp->firststring; ms; ms = ms->next)
			{
				if (!String::Length(ms->string))
				{
					newstrptr = strptr;
					break;
				}	//end if
					//Log_Write("MT_STRING: %s", mp->string);
				index = StringContains(strptr, ms->string, qfalse);
				if (index >= 0)
				{
					newstrptr = strptr + index;
					if (lastvariable >= 0)
					{
						match->variables[lastvariable].length =
							newstrptr - match->variables[lastvariable].ptr;
						lastvariable = -1;
						break;
					}	//end if
					else if (index == 0)
					{
						break;
					}	//end else
					newstrptr = NULL;
				}	//end if
			}	//end for
			if (!newstrptr)
			{
				return qfalse;
			}
			strptr = newstrptr + String::Length(ms->string);
		}	//end if
			//if it is a variable piece of string
		else if (mp->type == MT_VARIABLE)
		{
			//Log_Write("MT_VARIABLE");
			match->variables[mp->variable].ptr = strptr;
			lastvariable = mp->variable;
		}	//end else if
	}	//end for
		//if a match was found
	if (!mp && (lastvariable >= 0 || !String::Length(strptr)))
	{
		//if the last piece was a variable string
		if (lastvariable >= 0)
		{
			match->variables[lastvariable].length = String::Length(match->variables[lastvariable].ptr);
		}	//end if
		return qtrue;
	}	//end if
	return qfalse;
}	//end of the function StringsMatch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotFindMatch(char* str, bot_match_wolf_t* match, unsigned long int context)
{
	int i;
	bot_matchtemplate_t* ms;

	String::NCpy(match->string, str, MAX_MESSAGE_SIZE_WOLF);
	//remove any trailing enters
	while (String::Length(match->string) &&
		   match->string[String::Length(match->string) - 1] == '\n')
	{
		match->string[String::Length(match->string) - 1] = '\0';
	}	//end while
		//compare the string with all the match strings
	for (ms = matchtemplates; ms; ms = ms->next)
	{
		if (!(ms->context & context))
		{
			continue;
		}
		//reset the match variable pointers
		for (i = 0; i < MAX_MATCHVARIABLES; i++)
			match->variables[i].ptr = NULL;
		//
		if (StringsMatch(ms->first, match))
		{
			match->type = ms->type;
			match->subtype = ms->subtype;
			return qtrue;
		}	//end if
	}	//end for
	return qfalse;
}	//end of the function BotFindMatch
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotMatchVariable(bot_match_wolf_t* match, int variable, char* buf, int size)
{
	if (variable < 0 || variable >= MAX_MATCHVARIABLES)
	{
		BotImport_Print(PRT_FATAL, "BotMatchVariable: variable out of range\n");
		String::Cpy(buf, "");
		return;
	}	//end if

	if (match->variables[variable].ptr)
	{
		if (match->variables[variable].length < size)
		{
			size = match->variables[variable].length + 1;
		}
		String::NCpy(buf, match->variables[variable].ptr, size - 1);
		buf[size - 1] = '\0';
	}	//end if
	else
	{
		String::Cpy(buf, "");
	}	//end else
	return;
}	//end of the function BotMatchVariable
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_stringlist_t* BotFindStringInList(bot_stringlist_t* list, char* string)
{
	bot_stringlist_t* s;

	for (s = list; s; s = s->next)
	{
		if (!String::Cmp(s->string, string))
		{
			return s;
		}
	}	//end for
	return NULL;
}	//end of the function BotFindStringInList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_stringlist_t* BotCheckChatMessageIntegrety(char* message, bot_stringlist_t* stringlist)
{
	int i;
	char* msgptr;
	char temp[MAX_MESSAGE_SIZE_WOLF];
	bot_stringlist_t* s;

	msgptr = message;
	//
	while (*msgptr)
	{
		if (*msgptr == ESCAPE_CHAR)
		{
			msgptr++;
			switch (*msgptr)
			{
			case 'v':	//variable
			{
				//step over the 'v'
				msgptr++;
				while (*msgptr && *msgptr != ESCAPE_CHAR)
					msgptr++;
				//step over the trailing escape char
				if (*msgptr)
				{
					msgptr++;
				}
				break;
			}		//end case
			case 'r':	//random
			{
				//step over the 'r'
				msgptr++;
				for (i = 0; (*msgptr && *msgptr != ESCAPE_CHAR); i++)
				{
					temp[i] = *msgptr++;
				}		//end while
				temp[i] = '\0';
				//step over the trailing escape char
				if (*msgptr)
				{
					msgptr++;
				}
				//find the random keyword
				if (!RandomString(temp))
				{
					if (!BotFindStringInList(stringlist, temp))
					{
						Log_Write("%s = {\"%s\"} //MISSING RANDOM\r\n", temp, temp);
						s = (bot_stringlist_t*)GetClearedMemory(sizeof(bot_stringlist_t) + String::Length(temp) + 1);
						s->string = (char*)s + sizeof(bot_stringlist_t);
						String::Cpy(s->string, temp);
						s->next = stringlist;
						stringlist = s;
					}		//end if
				}		//end if
				break;
			}		//end case
			default:
			{
				BotImport_Print(PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message);
				break;
			}		//end default
			}	//end switch
		}	//end if
		else
		{
			msgptr++;
		}	//end else
	}	//end while
	return stringlist;
}	//end of the function BotCheckChatMessageIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotCheckReplyChatIntegrety(bot_replychat_t* replychat)
{
	bot_replychat_t* rp;
	bot_chatmessage_t* cm;
	bot_stringlist_t* stringlist, * s, * nexts;

	stringlist = NULL;
	for (rp = replychat; rp; rp = rp->next)
	{
		for (cm = rp->firstchatmessage; cm; cm = cm->next)
		{
			stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
		}	//end for
	}	//end for
	for (s = stringlist; s; s = nexts)
	{
		nexts = s->next;
		FreeMemory(s);
	}	//end for
}	//end of the function BotCheckReplyChatIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotCheckInitialChatIntegrety(bot_chat_t* chat)
{
	bot_chattype_t* t;
	bot_chatmessage_t* cm;
	bot_stringlist_t* stringlist, * s, * nexts;

	stringlist = NULL;
	for (t = chat->types; t; t = t->next)
	{
		for (cm = t->firstchatmessage; cm; cm = cm->next)
		{
			stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
		}	//end for
	}	//end for
	for (s = stringlist; s; s = nexts)
	{
		nexts = s->next;
		FreeMemory(s);
	}	//end for
}	//end of the function BotCheckInitialChatIntegrety
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotFreeReplyChat(bot_replychat_t* replychat)
{
	bot_replychat_t* rp, * nextrp;
	bot_replychatkey_t* key, * nextkey;
	bot_chatmessage_t* cm, * nextcm;

	for (rp = replychat; rp; rp = nextrp)
	{
		nextrp = rp->next;
		for (key = rp->keys; key; key = nextkey)
		{
			nextkey = key->next;
			if (key->match)
			{
				BotFreeMatchPieces(key->match);
			}
			if (key->string)
			{
				FreeMemory(key->string);
			}
			FreeMemory(key);
		}	//end for
		for (cm = rp->firstchatmessage; cm; cm = nextcm)
		{
			nextcm = cm->next;
			FreeMemory(cm);
		}	//end for
		FreeMemory(rp);
	}	//end for
}	//end of the function BotFreeReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_replychat_t* BotLoadReplyChat(const char* filename)
{
	char chatmessagestring[MAX_MESSAGE_SIZE_WOLF];
	char namebuffer[MAX_MESSAGE_SIZE_WOLF];
	source_t* source;
	token_t token;
	bot_chatmessage_t* chatmessage = NULL;
	bot_replychat_t* replychat, * replychatlist;
	bot_replychatkey_t* key;

	source = LoadSourceFile(filename);
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", filename);
		return NULL;
	}	//end if
		//
	replychatlist = NULL;
	//
	while (PC_ReadToken(source, &token))
	{
		if (String::Cmp(token.string, "["))
		{
			SourceError(source, "expected [, found %s", token.string);
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return NULL;
		}	//end if
			//
		replychat = (bot_replychat_t*)GetClearedHunkMemory(sizeof(bot_replychat_t));
		replychat->keys = NULL;
		replychat->next = replychatlist;
		replychatlist = replychat;
		//read the keys, there must be at least one key
		do
		{
			//allocate a key
			key = (bot_replychatkey_t*)GetClearedHunkMemory(sizeof(bot_replychatkey_t));
			key->flags = 0;
			key->string = NULL;
			key->match = NULL;
			key->next = replychat->keys;
			replychat->keys = key;
			//check for MUST BE PRESENT and MUST BE ABSENT keys
			if (PC_CheckTokenString(source, "&"))
			{
				key->flags |= RCKFL_AND;
			}
			else if (PC_CheckTokenString(source, "!"))
			{
				key->flags |= RCKFL_NOT;
			}
			//special keys
			if (PC_CheckTokenString(source, "name"))
			{
				key->flags |= RCKFL_NAME;
			}
			else if (PC_CheckTokenString(source, "female"))
			{
				key->flags |= RCKFL_GENDERFEMALE;
			}
			else if (PC_CheckTokenString(source, "male"))
			{
				key->flags |= RCKFL_GENDERMALE;
			}
			else if (PC_CheckTokenString(source, "it"))
			{
				key->flags |= RCKFL_GENDERLESS;
			}
			else if (PC_CheckTokenString(source, "("))																																																																		//match key
			{
				key->flags |= RCKFL_VARIABLES;
				key->match = BotLoadMatchPieces(source, ")");
				if (!key->match)
				{
					BotFreeReplyChat(replychatlist);
					return NULL;
				}	//end if
			}	//end else if
			else if (PC_CheckTokenString(source, "<"))		//bot names
			{
				key->flags |= RCKFL_BOTNAMES;
				String::Cpy(namebuffer, "");
				do
				{
					if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
					{
						BotFreeReplyChat(replychatlist);
						FreeSource(source);
						return NULL;
					}	//end if
					StripDoubleQuotes(token.string);
					if (String::Length(namebuffer))
					{
						strcat(namebuffer, "\\");
					}
					strcat(namebuffer, token.string);
				}
				while (PC_CheckTokenString(source, ","));
				if (!PC_ExpectTokenString(source, ">"))
				{
					BotFreeReplyChat(replychatlist);
					FreeSource(source);
					return NULL;
				}	//end if
				key->string = (char*)GetClearedHunkMemory(String::Length(namebuffer) + 1);
				String::Cpy(key->string, namebuffer);
			}	//end else if
			else//normal string key
			{
				key->flags |= RCKFL_STRING;
				if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
				{
					BotFreeReplyChat(replychatlist);
					FreeSource(source);
					return NULL;
				}	//end if
				StripDoubleQuotes(token.string);
				key->string = (char*)GetClearedHunkMemory(String::Length(token.string) + 1);
				String::Cpy(key->string, token.string);
			}	//end else
				//
			PC_CheckTokenString(source, ",");
		}
		while (!PC_CheckTokenString(source, "]"));
		//read the = sign and the priority
		if (!PC_ExpectTokenString(source, "=") ||
			!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
		{
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return NULL;
		}	//end if
		replychat->priority = token.floatvalue;
		//read the leading {
		if (!PC_ExpectTokenString(source, "{"))
		{
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return NULL;
		}	//end if
		replychat->numchatmessages = 0;
		//while the trailing } is not found
		while (!PC_CheckTokenString(source, "}"))
		{
			if (!BotLoadChatMessage(source, chatmessagestring))
			{
				BotFreeReplyChat(replychatlist);
				FreeSource(source);
				return NULL;
			}	//end if
			chatmessage = (bot_chatmessage_t*)GetClearedHunkMemory(sizeof(bot_chatmessage_t) + String::Length(chatmessagestring) + 1);
			chatmessage->chatmessage = (char*)chatmessage + sizeof(bot_chatmessage_t);
			String::Cpy(chatmessage->chatmessage, chatmessagestring);
			chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
			chatmessage->next = replychat->firstchatmessage;
			//add the chat message to the reply chat
			replychat->firstchatmessage = chatmessage;
			replychat->numchatmessages++;
		}	//end while
	}	//end while
	FreeSource(source);
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);
	//
	if (bot_developer)
	{
		BotCheckReplyChatIntegrety(replychatlist);
	}	//end if
		//
	if (!replychatlist)
	{
		BotImport_Print(PRT_MESSAGE, "no rchats\n");
	}
	//
	return replychatlist;
}	//end of the function BotLoadReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotDumpInitialChat(bot_chat_t* chat)
{
	bot_chattype_t* t;
	bot_chatmessage_t* m;

	Log_Write("{");
	for (t = chat->types; t; t = t->next)
	{
		Log_Write(" type \"%s\"", t->name);
		Log_Write(" {");
		Log_Write("  numchatmessages = %d", t->numchatmessages);
		for (m = t->firstchatmessage; m; m = m->next)
		{
			Log_Write("  \"%s\"", m->chatmessage);
		}	//end for
		Log_Write(" }");
	}	//end for
	Log_Write("}");
}	//end of the function BotDumpInitialChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bot_chat_t* BotLoadInitialChat(char* chatfile, char* chatname)
{
	int pass, foundchat, indent, size;
	char* ptr = NULL;
	char chatmessagestring[MAX_MESSAGE_SIZE_WOLF];
	source_t* source;
	token_t token;
	bot_chat_t* chat = NULL;
	bot_chattype_t* chattype = NULL;
	bot_chatmessage_t* chatmessage = NULL;
#ifdef DEBUG
	int starttime;

	starttime = Sys_Milliseconds();
#endif	//DEBUG
		//
	size = 0;
	foundchat = qfalse;
	//a bot chat is parsed in two phases
	for (pass = 0; pass < 2; pass++)
	{
		//allocate memory
		if (pass && size)
		{
			ptr = (char*)GetClearedMemory(size);
		}
		//load the source file
		source = LoadSourceFile(chatfile);
		if (!source)
		{
			BotImport_Print(PRT_ERROR, "counldn't load %s\n", chatfile);
			return NULL;
		}	//end if
			//chat structure
		if (pass)
		{
			chat = (bot_chat_t*)ptr;
			ptr += sizeof(bot_chat_t);
		}	//end if
		size = sizeof(bot_chat_t);
		//
		while (PC_ReadToken(source, &token))
		{
			if (!String::Cmp(token.string, "chat"))
			{
				if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
				{
					FreeSource(source);
					return NULL;
				}	//end if
				StripDoubleQuotes(token.string);
				//after the chat name we expect a opening brace
				if (!PC_ExpectTokenString(source, "{"))
				{
					FreeSource(source);
					return NULL;
				}	//end if
					//if the chat name is found
				if (!String::ICmp(token.string, chatname))
				{
					foundchat = qtrue;
					//read the chat types
					while (1)
					{
						if (!PC_ExpectAnyToken(source, &token))
						{
							FreeSource(source);
							return NULL;
						}	//end if
						if (!String::Cmp(token.string, "}"))
						{
							break;
						}
						if (String::Cmp(token.string, "type"))
						{
							SourceError(source, "expected type found %s\n", token.string);
							FreeSource(source);
							return NULL;
						}	//end if
							//expect the chat type name
						if (!PC_ExpectTokenType(source, TT_STRING, 0, &token) ||
							!PC_ExpectTokenString(source, "{"))
						{
							FreeSource(source);
							return NULL;
						}	//end if
						StripDoubleQuotes(token.string);
						if (pass)
						{
							chattype = (bot_chattype_t*)ptr;
							String::NCpy(chattype->name, token.string, MAX_CHATTYPE_NAME);
							chattype->firstchatmessage = NULL;
							//add the chat type to the chat
							chattype->next = chat->types;
							chat->types = chattype;
							//
							ptr += sizeof(bot_chattype_t);
						}	//end if
						size += sizeof(bot_chattype_t);
						//read the chat messages
						while (!PC_CheckTokenString(source, "}"))
						{
							if (!BotLoadChatMessage(source, chatmessagestring))
							{
								FreeSource(source);
								return NULL;
							}	//end if
							if (pass)
							{
								chatmessage = (bot_chatmessage_t*)ptr;
								chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
								//put the chat message in the list
								chatmessage->next = chattype->firstchatmessage;
								chattype->firstchatmessage = chatmessage;
								//store the chat message
								ptr += sizeof(bot_chatmessage_t);
								chatmessage->chatmessage = ptr;
								String::Cpy(chatmessage->chatmessage, chatmessagestring);
								ptr += String::Length(chatmessagestring) + 1;
								//the number of chat messages increased
								chattype->numchatmessages++;
							}	//end if
							size += sizeof(bot_chatmessage_t) + String::Length(chatmessagestring) + 1;
						}	//end if
					}	//end while
				}	//end if
				else//skip the bot chat
				{
					indent = 1;
					while (indent)
					{
						if (!PC_ExpectAnyToken(source, &token))
						{
							FreeSource(source);
							return NULL;
						}	//end if
						if (!String::Cmp(token.string, "{"))
						{
							indent++;
						}
						else if (!String::Cmp(token.string, "}"))
						{
							indent--;
						}
					}	//end while
				}	//end else
			}	//end if
			else
			{
				SourceError(source, "unknown definition %s\n", token.string);
				FreeSource(source);
				return NULL;
			}	//end else
		}	//end while
			//free the source
		FreeSource(source);
		//if the requested character is not found
		if (!foundchat)
		{
			BotImport_Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, chatfile);
			return NULL;
		}	//end if
	}	//end for
		//
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile);
#endif
	//
	//BotDumpInitialChat(chat);
	if (bot_developer)
	{
		BotCheckInitialChatIntegrety(chat);
	}	//end if
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "initial chats loaded in %d msec\n", Sys_Milliseconds() - starttime);
#endif	//DEBUG
		//character was read succesfully
	return chat;
}	//end of the function BotLoadInitialChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotFreeChatFile(int chatstate)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	if (cs->chat)
	{
		FreeMemory(cs->chat);
	}
	cs->chat = NULL;
}	//end of the function BotFreeChatFile
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotLoadChatFile(int chatstate, char* chatfile, char* chatname)
{
	bot_chatstate_t* cs;
	int n, avail = 0;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return WOLFBLERR_CANNOTLOADICHAT;
	}
	BotFreeChatFile(chatstate);

	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		avail = -1;
		for (n = 0; n < MAX_CLIENTS_ET; n++)
		{
			if (!ichatdata[n])
			{
				if (avail == -1)
				{
					avail = n;
				}
				continue;
			}
			if (String::Cmp(chatfile, ichatdata[n]->filename) != 0)
			{
				continue;
			}
			if (String::Cmp(chatname, ichatdata[n]->chatname) != 0)
			{
				continue;
			}
			cs->chat = ichatdata[n]->chat;
			//		BotImport_Print( PRT_MESSAGE, "retained %s from %s\n", chatname, chatfile );
			return BLERR_NOERROR;
		}

		if (avail == -1)
		{
			BotImport_Print(PRT_FATAL, "ichatdata table full; couldn't load chat %s from %s\n", chatname, chatfile);
			return WOLFBLERR_CANNOTLOADICHAT;
		}
	}

	PS_SetBaseFolder("botfiles");
	cs->chat = BotLoadInitialChat(chatfile, chatname);
	PS_SetBaseFolder("");
	if (!cs->chat)
	{
		BotImport_Print(PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile);
		return WOLFBLERR_CANNOTLOADICHAT;
	}	//end if
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		ichatdata[avail] = (bot_ichatdata_t*)Mem_Alloc(sizeof(bot_ichatdata_t));
		ichatdata[avail]->chat = cs->chat;
		String::NCpyZ(ichatdata[avail]->chatname, chatname, sizeof(ichatdata[avail]->chatname));
		String::NCpyZ(ichatdata[avail]->filename, chatfile, sizeof(ichatdata[avail]->filename));
	}	//end if

	return BLERR_NOERROR;
}	//end of the function BotLoadChatFile
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BotExpandChatMessage(char* outmessage, char* message, unsigned long mcontext,
	bot_matchvariable_wolf_t* variables, unsigned long vcontext, int reply)
{
	int num, len, i, expansion;
	char* outputbuf, * ptr, * msgptr;
	char temp[MAX_MESSAGE_SIZE_WOLF];

	expansion = qfalse;
	msgptr = message;
	outputbuf = outmessage;
	len = 0;
	//
	while (*msgptr)
	{
		if (*msgptr == ESCAPE_CHAR)
		{
			msgptr++;
			switch (*msgptr)
			{
			case 'v':	//variable
			{
				msgptr++;
				num = 0;
				while (*msgptr && *msgptr != ESCAPE_CHAR)
				{
					num = num * 10 + (*msgptr++) - '0';
				}		//end while
						//step over the trailing escape char
				if (*msgptr)
				{
					msgptr++;
				}
				if (num > MAX_MATCHVARIABLES)
				{
					BotImport_Print(PRT_ERROR, "BotConstructChat: message %s variable %d out of range\n", message, num);
					return qfalse;
				}		//end if
				ptr = variables[num].ptr;
				if (ptr)
				{
					for (i = 0; i < variables[num].length; i++)
					{
						temp[i] = ptr[i];
					}		//end for
					temp[i] = 0;
					//if it's a reply message
					if (reply)
					{
						//replace the reply synonyms in the variables
						BotReplaceReplySynonyms(temp, vcontext);
					}		//end if
					else
					{
						//replace synonyms in the variable context
						BotReplaceSynonyms(temp, vcontext);
					}		//end else
							//
					if (len + String::Length(temp) >= MAX_MESSAGE_SIZE_WOLF)
					{
						BotImport_Print(PRT_ERROR, "BotConstructChat: message %s too long\n", message);
						return qfalse;
					}		//end if
					String::Cpy(&outputbuf[len], temp);
					len += String::Length(temp);
				}		//end if
				break;
			}		//end case
			case 'r':	//random
			{
				msgptr++;
				for (i = 0; (*msgptr && *msgptr != ESCAPE_CHAR); i++)
				{
					temp[i] = *msgptr++;
				}		//end while
				temp[i] = '\0';
				//step over the trailing escape char
				if (*msgptr)
				{
					msgptr++;
				}
				//find the random keyword
				ptr = RandomString(temp);
				if (!ptr)
				{
					BotImport_Print(PRT_ERROR, "BotConstructChat: unknown random string %s\n", temp);
					return qfalse;
				}		//end if
				if (len + String::Length(ptr) >= MAX_MESSAGE_SIZE_WOLF)
				{
					BotImport_Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
					return qfalse;
				}		//end if
				String::Cpy(&outputbuf[len], ptr);
				len += String::Length(ptr);
				expansion = qtrue;
				break;
			}		//end case
			default:
			{
				BotImport_Print(PRT_FATAL, "BotConstructChat: message \"%s\" invalid escape char\n", message);
				break;
			}		//end default
			}	//end switch
		}	//end if
		else
		{
			outputbuf[len++] = *msgptr++;
			if (len >= MAX_MESSAGE_SIZE_WOLF)
			{
				BotImport_Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
				break;
			}	//end if
		}	//end else
	}	//end while
	outputbuf[len] = '\0';
	//replace synonyms weighted in the message context
	BotReplaceWeightedSynonyms(outputbuf, mcontext);
	//return true if a random was expanded
	return expansion;
}	//end of the function BotExpandChatMessage
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotConstructChatMessage(bot_chatstate_t* chatstate, char* message, unsigned long mcontext,
	bot_matchvariable_wolf_t* variables, unsigned long vcontext, int reply)
{
	int i;
	char srcmessage[MAX_MESSAGE_SIZE_WOLF];

	String::Cpy(srcmessage, message);
	for (i = 0; i < 10; i++)
	{
		if (!BotExpandChatMessage(chatstate->chatmessage, srcmessage, mcontext, variables, vcontext, reply))
		{
			break;
		}	//end if
		String::Cpy(srcmessage, chatstate->chatmessage);
	}	//end for
	if (i >= 10)
	{
		BotImport_Print(PRT_WARNING, "too many expansions in chat message\n");
		BotImport_Print(PRT_WARNING, "%s\n", chatstate->chatmessage);
	}	//end if
}	//end of the function BotConstructChatMessage
//===========================================================================
// randomly chooses one of the chat message of the given type
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char* BotChooseInitialChatMessage(bot_chatstate_t* cs, char* type)
{
	int n, numchatmessages;
	float besttime;
	bot_chattype_t* t;
	bot_chatmessage_t* m, * bestchatmessage;
	bot_chat_t* chat;

	chat = cs->chat;
	for (t = chat->types; t; t = t->next)
	{
		if (!String::ICmp(t->name, type))
		{
			numchatmessages = 0;
			for (m = t->firstchatmessage; m; m = m->next)
			{
				if (m->time > AAS_Time())
				{
					continue;
				}
				numchatmessages++;
			}	//end if
				//if all chat messages have been used recently
			if (numchatmessages <= 0)
			{
				besttime = 0;
				bestchatmessage = NULL;
				for (m = t->firstchatmessage; m; m = m->next)
				{
					if (!besttime || m->time < besttime)
					{
						bestchatmessage = m;
						besttime = m->time;
					}	//end if
				}	//end for
				if (bestchatmessage)
				{
					return bestchatmessage->chatmessage;
				}
			}	//end if
			else//choose a chat message randomly
			{
				n = random() * numchatmessages;
				for (m = t->firstchatmessage; m; m = m->next)
				{
					if (m->time > AAS_Time())
					{
						continue;
					}
					if (--n < 0)
					{
						m->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
						return m->chatmessage;
					}	//end if
				}	//end for
			}	//end else
			return NULL;
		}	//end if
	}	//end for
	return NULL;
}	//end of the function BotChooseInitialChatMessage
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotNumInitialChats(int chatstate, char* type)
{
	bot_chatstate_t* cs;
	bot_chattype_t* t;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}

	for (t = cs->chat->types; t; t = t->next)
	{
		if (!String::ICmp(t->name, type))
		{
			if (LibVarGetValue("bot_testichat"))
			{
				BotImport_Print(PRT_MESSAGE, "%s has %d chat lines\n", type, t->numchatmessages);
				BotImport_Print(PRT_MESSAGE, "-------------------\n");
			}
			return t->numchatmessages;
		}	//end if
	}	//end for
	return 0;
}	//end of the function BotNumInitialChats
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotInitialChat(int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
	char* message;
	bot_matchvariable_wolf_t variables[MAX_MATCHVARIABLES];
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	//if no chat file is loaded
	if (!cs->chat)
	{
		return;
	}
	//choose a chat message randomly of the given type
	message = BotChooseInitialChatMessage(cs, type);
	//if there's no message of the given type
	if (!message)
	{
#ifdef DEBUG
		BotImport_Print(PRT_MESSAGE, "no chat messages of type %s\n", type);
#endif	//DEBUG
		return;
	}	//end if
		//
	memset(variables, 0, sizeof(variables));
	if (var0)
	{
		variables[0].ptr = var0;
		variables[0].length = String::Length(var0);
	}
	if (var1)
	{
		variables[1].ptr = var1;
		variables[1].length = String::Length(var1);
	}
	if (var2)
	{
		variables[2].ptr = var2;
		variables[2].length = String::Length(var2);
	}
	if (var3)
	{
		variables[3].ptr = var3;
		variables[3].length = String::Length(var3);
	}
	if (var4)
	{
		variables[4].ptr = var4;
		variables[4].length = String::Length(var4);
	}
	if (var5)
	{
		variables[5].ptr = var5;
		variables[5].length = String::Length(var5);
	}
	if (var6)
	{
		variables[6].ptr = var6;
		variables[6].length = String::Length(var6);
	}
	if (var7)
	{
		variables[7].ptr = var7;
		variables[7].length = String::Length(var7);
	}
	//
	BotConstructChatMessage(cs, message, mcontext, variables, 0, qfalse);
}	//end of the function BotInitialChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotPrintReplyChatKeys(bot_replychat_t* replychat)
{
	bot_replychatkey_t* key;
	bot_matchpiece_t* mp;

	BotImport_Print(PRT_MESSAGE, "[");
	for (key = replychat->keys; key; key = key->next)
	{
		if (key->flags & RCKFL_AND)
		{
			BotImport_Print(PRT_MESSAGE, "&");
		}
		else if (key->flags & RCKFL_NOT)
		{
			BotImport_Print(PRT_MESSAGE, "!");
		}
		//
		if (key->flags & RCKFL_NAME)
		{
			BotImport_Print(PRT_MESSAGE, "name");
		}
		else if (key->flags & RCKFL_GENDERFEMALE)
		{
			BotImport_Print(PRT_MESSAGE, "female");
		}
		else if (key->flags & RCKFL_GENDERMALE)
		{
			BotImport_Print(PRT_MESSAGE, "male");
		}
		else if (key->flags & RCKFL_GENDERLESS)
		{
			BotImport_Print(PRT_MESSAGE, "it");
		}
		else if (key->flags & RCKFL_VARIABLES)
		{
			BotImport_Print(PRT_MESSAGE, "(");
			for (mp = key->match; mp; mp = mp->next)
			{
				if (mp->type == MT_STRING)
				{
					BotImport_Print(PRT_MESSAGE, "\"%s\"", mp->firststring->string);
				}
				else
				{
					BotImport_Print(PRT_MESSAGE, "%d", mp->variable);
				}
				if (mp->next)
				{
					BotImport_Print(PRT_MESSAGE, ", ");
				}
			}	//end for
			BotImport_Print(PRT_MESSAGE, ")");
		}	//end if
		else if (key->flags & RCKFL_STRING)
		{
			BotImport_Print(PRT_MESSAGE, "\"%s\"", key->string);
		}	//end if
		if (key->next)
		{
			BotImport_Print(PRT_MESSAGE, ", ");
		}
		else
		{
			BotImport_Print(PRT_MESSAGE, "] = %1.0f\n", replychat->priority);
		}
	}	//end for
	BotImport_Print(PRT_MESSAGE, "{\n");
}	//end of the function BotPrintReplyChatKeys
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotReplyChat(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
	bot_replychat_t* rchat, * bestrchat;
	bot_replychatkey_t* key;
	bot_chatmessage_t* m, * bestchatmessage;
	bot_match_wolf_t match, bestmatch;
	int bestpriority, num, found, res, numchatmessages;
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return qfalse;
	}
	memset(&match, 0, sizeof(bot_match_wolf_t));
	String::Cpy(match.string, message);
	bestpriority = -1;
	bestchatmessage = NULL;
	bestrchat = NULL;
	//go through all the reply chats
	for (rchat = replychats; rchat; rchat = rchat->next)
	{
		found = qfalse;
		for (key = rchat->keys; key; key = key->next)
		{
			res = qfalse;
			//get the match result
			if (key->flags & RCKFL_NAME)
			{
				res = (StringContains(message, cs->name, qfalse) != -1);
			}
			else if (key->flags & RCKFL_BOTNAMES)
			{
				res = (StringContains(key->string, cs->name, qfalse) != -1);
			}
			else if (key->flags & RCKFL_GENDERFEMALE)
			{
				res = (cs->gender == CHAT_GENDERFEMALE);
			}
			else if (key->flags & RCKFL_GENDERMALE)
			{
				res = (cs->gender == CHAT_GENDERMALE);
			}
			else if (key->flags & RCKFL_GENDERLESS)
			{
				res = (cs->gender == CHAT_GENDERLESS);
			}
			else if (key->flags & RCKFL_VARIABLES)
			{
				res = StringsMatch(key->match, &match);
			}
			else if (key->flags & RCKFL_STRING)
			{
				res = (StringContainsWord(message, key->string, qfalse) != NULL);
			}
			//if the key must be present
			if (key->flags & RCKFL_AND)
			{
				if (!res)
				{
					found = qfalse;
					break;
				}	//end if
					//botnames is an exception
					//if (!(key->flags & RCKFL_BOTNAMES)) found = qtrue;
			}	//end else if
				//if the key must be absent
			else if (key->flags & RCKFL_NOT)
			{
				if (res)
				{
					found = qfalse;
					break;
				}	//end if
			}	//end if
			else if (res)
			{
				found = qtrue;
			}	//end else
		}	//end for
			//
		if (found)
		{
			if (rchat->priority > bestpriority)
			{
				numchatmessages = 0;
				for (m = rchat->firstchatmessage; m; m = m->next)
				{
					if (m->time > AAS_Time())
					{
						continue;
					}
					numchatmessages++;
				}	//end if
				num = random() * numchatmessages;
				for (m = rchat->firstchatmessage; m; m = m->next)
				{
					if (--num < 0)
					{
						break;
					}
					if (m->time > AAS_Time())
					{
						continue;
					}
				}	//end for
					//if the reply chat has a message
				if (m)
				{
					memcpy(&bestmatch, &match, sizeof(bot_match_wolf_t));
					bestchatmessage = m;
					bestrchat = rchat;
					bestpriority = rchat->priority;
				}	//end if
			}	//end if
		}	//end if
	}	//end for
	if (bestchatmessage)
	{
		if (var0)
		{
			bestmatch.variables[0].ptr = var0;
			bestmatch.variables[0].length = String::Length(var0);
		}
		if (var1)
		{
			bestmatch.variables[1].ptr = var1;
			bestmatch.variables[1].length = String::Length(var1);
		}
		if (var2)
		{
			bestmatch.variables[2].ptr = var2;
			bestmatch.variables[2].length = String::Length(var2);
		}
		if (var3)
		{
			bestmatch.variables[3].ptr = var3;
			bestmatch.variables[3].length = String::Length(var3);
		}
		if (var4)
		{
			bestmatch.variables[4].ptr = var4;
			bestmatch.variables[4].length = String::Length(var4);
		}
		if (var5)
		{
			bestmatch.variables[5].ptr = var5;
			bestmatch.variables[5].length = String::Length(var5);
		}
		if (var6)
		{
			bestmatch.variables[6].ptr = var6;
			bestmatch.variables[6].length = String::Length(var6);
		}
		if (var7)
		{
			bestmatch.variables[7].ptr = var7;
			bestmatch.variables[7].length = String::Length(var7);
		}
		if (LibVarGetValue("bot_testrchat"))
		{
			for (m = bestrchat->firstchatmessage; m; m = m->next)
			{
				BotConstructChatMessage(cs, m->chatmessage, mcontext, bestmatch.variables, vcontext, qtrue);
				BotRemoveTildes(cs->chatmessage);
				BotImport_Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
			}	//end if
		}	//end if
		else
		{
			bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
			BotConstructChatMessage(cs, bestchatmessage->chatmessage, mcontext, bestmatch.variables, vcontext, qtrue);
		}	//end else
		return qtrue;
	}	//end if
	return qfalse;
}	//end of the function BotReplyChat
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotChatLength(int chatstate)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}
	return String::Length(cs->chatmessage);
}	//end of the function BotChatLength
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotEnterChat(int chatstate, int client, int sendto)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}

	if (String::Length(cs->chatmessage))
	{
		BotRemoveTildes(cs->chatmessage);
		if (LibVarGetValue("bot_testichat"))
		{
			BotImport_Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
		}
		else
		{
			if (sendto == CHAT_TEAM)
			{
				EA_SayTeam(client, cs->chatmessage);
			}
			else
			{
				EA_Say(client, cs->chatmessage);
			}
		}
		//clear the chat message from the state
		String::Cpy(cs->chatmessage, "");
	}	//end if
}	//end of the function BotEnterChat
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotGetChatMessage(int chatstate, char* buf, int size)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}

	BotRemoveTildes(cs->chatmessage);
	String::NCpy(buf, cs->chatmessage, size - 1);
	buf[size - 1] = '\0';
	//clear the chat message from the state
	String::Cpy(cs->chatmessage, "");
}	//end of the function BotGetChatMessage
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BotSetChatGender(int chatstate, int gender)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	switch (gender)
	{
	case CHAT_GENDERFEMALE: cs->gender = CHAT_GENDERFEMALE; break;
	case CHAT_GENDERMALE: cs->gender = CHAT_GENDERMALE; break;
	default: cs->gender = CHAT_GENDERLESS; break;
	}	//end switch
}	//end of the function BotSetChatGender
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotSetChatName(int chatstate, char* name)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	memset(cs->name, 0, sizeof(cs->name));
	String::NCpy(cs->name, name, sizeof(cs->name));
	cs->name[sizeof(cs->name) - 1] = '\0';
}	//end of the function BotSetChatName
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotResetChatAI(void)
{
	bot_replychat_t* rchat;
	bot_chatmessage_t* m;

	for (rchat = replychats; rchat; rchat = rchat->next)
	{
		for (m = rchat->firstchatmessage; m; m = m->next)
		{
			m->time = 0;
		}	//end for
	}	//end for
}	//end of the function BotResetChatAI
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
int BotAllocChatState(void)
{
	int i;

	for (i = 1; i <= MAX_CLIENTS_ET; i++)
	{
		if (!botchatstates[i])
		{
			botchatstates[i] = (bot_chatstate_t*)GetClearedMemory(sizeof(bot_chatstate_t));
			return i;
		}	//end if
	}	//end for
	return 0;
}	//end of the function BotAllocChatState
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void BotFreeChatState(int handle)
{
	bot_chatstate_t* cs;
	bot_consolemessage_wolf_t m;
	int h;

	if (handle <= 0 || handle > MAX_CLIENTS_ET)
	{
		BotImport_Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
		return;
	}	//end if
	if (!botchatstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid chat state %d\n", handle);
		return;
	}	//end if
	cs = botchatstates[handle];
	if (LibVarGetValue("bot_reloadcharacters"))
	{
		BotFreeChatFile(handle);
	}	//end if
		//free all the console messages left in the chat state
	for (h = BotNextConsoleMessageWolf(handle, &m); h; h = BotNextConsoleMessageWolf(handle, &m))
	{
		//remove the console message
		BotRemoveConsoleMessage(handle, h);
	}	//end for
	FreeMemory(botchatstates[handle]);
	botchatstates[handle] = NULL;
}	//end of the function BotFreeChatState
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BotSetupChatAI(void)
{
	const char* file;

#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif	//DEBUG

	PS_SetBaseFolder("botfiles");
	file = LibVarString("synfile", "syn.c");
	synonyms = BotLoadSynonyms(file);
	file = LibVarString("rndfile", "rnd.c");
	randomstrings = BotLoadRandomStrings(file);
	file = LibVarString("matchfile", "match.c");
	matchtemplates = BotLoadMatchTemplates(file);
	//
	if (!LibVarValue("nochat", "0"))
	{
		file = LibVarString("rchatfile", "rchat.c");
		replychats = BotLoadReplyChat(file);
	}	//end if
	PS_SetBaseFolder("");

	InitConsoleMessageHeap();

#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "setup chat AI %d msec\n", Sys_Milliseconds() - starttime);
#endif	//DEBUG
	return BLERR_NOERROR;
}	//end of the function BotSetupChatAI
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BotShutdownChatAI(void)
{
	int i;

	//free all remaining chat states
	for (i = 0; i < MAX_CLIENTS_ET; i++)
	{
		if (botchatstates[i])
		{
			BotFreeChatState(i);
		}	//end if
	}	//end for
		//free all cached chats
	for (i = 0; i < MAX_CLIENTS_ET; i++)
	{
		if (ichatdata[i])
		{
			FreeMemory(ichatdata[i]->chat);
			Mem_Free(ichatdata[i]);
			ichatdata[i] = NULL;
		}	//end if
	}	//end for
	if (consolemessageheap)
	{
		Mem_Free(consolemessageheap);
	}
	consolemessageheap = NULL;
	if (matchtemplates)
	{
		BotFreeMatchTemplates(matchtemplates);
	}
	matchtemplates = NULL;
	if (randomstrings)
	{
		FreeMemory(randomstrings);
	}
	randomstrings = NULL;
	if (synonyms)
	{
		FreeMemory(synonyms);
	}
	synonyms = NULL;
	if (replychats)
	{
		BotFreeReplyChat(replychats);
	}
	replychats = NULL;
}	//end of the function BotShutdownChatAI
