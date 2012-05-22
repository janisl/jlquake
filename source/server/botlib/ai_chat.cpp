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

#include "../server.h"
#include "local.h"

bot_chatstate_t* botchatstates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
//console message heap
bot_consolemessage_t* consolemessageheap = NULL;
bot_consolemessage_t* freeconsolemessages = NULL;
//list with match strings
bot_matchtemplate_t* matchtemplates = NULL;
//list with synonyms
bot_synonymlist_t* synonyms = NULL;
//list with random strings
bot_randomlist_t* randomstrings = NULL;
//reply chats
bot_replychat_t* replychats = NULL;
bot_ichatdata_t* ichatdata[MAX_BOTLIB_CLIENTS_ARRAY];

bot_chatstate_t* BotChatStateFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
		return NULL;
	}
	if (!botchatstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid chat state %d\n", handle);
		return NULL;
	}
	return botchatstates[handle];
}

// initialize the heap with unused console messages
void InitConsoleMessageHeap()
{
	int i, max_messages;

	if (consolemessageheap)
	{
		Mem_Free(consolemessageheap);
	}

	max_messages = (int)LibVarValue("max_messages", "1024");
	consolemessageheap = (bot_consolemessage_t*)Mem_ClearedAlloc(max_messages *
		sizeof(bot_consolemessage_t));
	consolemessageheap[0].prev = NULL;
	consolemessageheap[0].next = &consolemessageheap[1];
	for (i = 1; i < max_messages - 1; i++)
	{
		consolemessageheap[i].prev = &consolemessageheap[i - 1];
		consolemessageheap[i].next = &consolemessageheap[i + 1];
	}
	consolemessageheap[max_messages - 1].prev = &consolemessageheap[max_messages - 2];
	consolemessageheap[max_messages - 1].next = NULL;
	//pointer to the free console messages
	freeconsolemessages = consolemessageheap;
}

// allocate one console message from the heap
static bot_consolemessage_t* AllocConsoleMessage()
{
	bot_consolemessage_t* message;
	message = freeconsolemessages;
	if (freeconsolemessages)
	{
		freeconsolemessages = freeconsolemessages->next;
	}
	if (freeconsolemessages)
	{
		freeconsolemessages->prev = NULL;
	}
	return message;
}

// deallocate one console message from the heap
void FreeConsoleMessage(bot_consolemessage_t* message)
{
	if (freeconsolemessages)
	{
		freeconsolemessages->prev = message;
	}
	message->prev = NULL;
	message->next = freeconsolemessages;
	freeconsolemessages = message;
}

void BotRemoveConsoleMessage(int chatstate, int handle)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}

	bot_consolemessage_t* nextm;
	for (bot_consolemessage_t* m = cs->firstmessage; m; m = nextm)
	{
		nextm = m->next;
		if (m->handle == handle)
		{
			if (m->next)
			{
				m->next->prev = m->prev;
			}
			else
			{
				cs->lastmessage = m->prev;
			}
			if (m->prev)
			{
				m->prev->next = m->next;
			}
			else
			{
				cs->firstmessage = m->next;
			}

			FreeConsoleMessage(m);
			cs->numconsolemessages--;
			break;
		}
	}
}

void BotQueueConsoleMessage(int chatstate, int type, const char* message)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}

	bot_consolemessage_t* m = AllocConsoleMessage();
	if (!m)
	{
		BotImport_Print(PRT_ERROR, "empty console message heap\n");
		return;
	}
	cs->handle++;
	if (cs->handle <= 0 || cs->handle > 8192)
	{
		cs->handle = 1;
	}
	m->handle = cs->handle;
	m->time = AAS_Time();
	m->type = type;
	String::NCpyZ(m->message, message, MAX_MESSAGE_SIZE);
	m->next = NULL;
	if (cs->lastmessage)
	{
		cs->lastmessage->next = m;
		m->prev = cs->lastmessage;
		cs->lastmessage = m;
	}
	else
	{
		cs->lastmessage = m;
		cs->firstmessage = m;
		m->prev = NULL;
	}
	cs->numconsolemessages++;
}

int BotNextConsoleMessageQ3(int chatstate, bot_consolemessage_q3_t* cm)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}
	if (cs->firstmessage)
	{
		cm->handle = cs->firstmessage->handle;
		cm->time = cs->firstmessage->time;
		cm->type = cs->firstmessage->type;
		String::NCpyZ(cm->message, cs->firstmessage->message, MAX_MESSAGE_SIZE_Q3);
		cm->next = 0;
		cm->prev = 0;
		return cm->handle;
	}
	return 0;
}

int BotNextConsoleMessageWolf(int chatstate, bot_consolemessage_wolf_t* cm)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}
	if (cs->firstmessage)
	{
		cm->handle = cs->firstmessage->handle;
		cm->time = cs->firstmessage->time;
		cm->type = cs->firstmessage->type;
		String::NCpyZ(cm->message, cs->firstmessage->message, MAX_MESSAGE_SIZE_WOLF);
		cm->next = NULL;
		cm->prev = NULL;
		return cm->handle;
	}
	return 0;
}

int BotNumConsoleMessages(int chatstate)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}
	return cs->numconsolemessages;
}

static int IsWhiteSpace(char c)
{
	if ((c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9') ||
		c == '(' || c == ')' ||
		c == '?' || c == ':' ||
		c == '\'' || c == '/' ||
		c == ',' || c == '.' ||
		c == '[' || c == ']' ||
		c == '-' || c == '_' ||
		c == '+' || c == '=')
	{
		return false;
	}
	return true;
}

void UnifyWhiteSpaces(char* string)
{
	for (char* ptr = string; *ptr;)
	{
		char* oldptr = ptr;
		while (*ptr && IsWhiteSpace(*ptr))
		{
			ptr++;
		}
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
		}
		while (*ptr && !IsWhiteSpace(*ptr))
		{
			ptr++;
		}
	}
}

void BotRemoveTildes(char* message)
{
	//remove all tildes from the chat message
	for (int i = 0; message[i]; i++)
	{
		if (message[i] == '~')
		{
			memmove(&message[i], &message[i + 1], String::Length(&message[i + 1]) + 1);
		}
	}
}

int StringContains(const char* str1, const char* str2, bool casesensitive)
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
			}
			else
			{
				if (String::ToUpper(str1[j]) != String::ToUpper(str2[j]))
				{
					break;
				}
			}
		}
		if (!str2[j])
		{
			return index;
		}
	}
	return -1;
}

char* StringContainsWord(char* str1, const char* str2, bool casesensitive)
{
	int len = String::Length(str1) - String::Length(str2);
	for (int i = 0; i <= len; i++, str1++)
	{
		//if not at the start of the string
		if (i)
		{
			//skip to the start of the next word
			while (*str1 && *str1 != ' ' && *str1 != '.' && *str1 != ',' && *str1 != '!')
				str1++;
			if (!*str1)
			{
				break;
			}
			str1++;
		}
		//compare the word
		int j;
		for (j = 0; str2[j]; j++)
		{
			if (casesensitive)
			{
				if (str1[j] != str2[j])
				{
					break;
				}
			}
			else
			{
				if (String::ToUpper(str1[j]) != String::ToUpper(str2[j]))
				{
					break;
				}
			}
		}
		//if there was a word match
		if (!str2[j])
		{
			//if the first string has an end of word
			if (!str1[j] || str1[j] == ' ' || str1[j] == '.' || str1[j] == ',' || str1[j] == '!')
			{
				return str1;
			}
		}
	}
	return NULL;
}

static void StringReplaceWords(char* string, const char* synonym, const char* replacement)
{
	//find the synonym in the string
	char* str = StringContainsWord(string, synonym, false);
	//if the synonym occured in the string
	while (str)
	{
		//if the synonym isn't part of the replacement which is already in the string
		//usefull for abreviations
		char* str2 = StringContainsWord(string, replacement, false);
		while (str2)
		{
			if (str2 <= str && str < str2 + String::Length(replacement))
			{
				break;
			}
			str2 = StringContainsWord(str2 + 1, replacement, false);
		}
		if (!str2)
		{
			memmove(str + String::Length(replacement), str + String::Length(synonym), String::Length(str + String::Length(synonym)) + 1);
			//append the synonum replacement
			Com_Memcpy(str, replacement, String::Length(replacement));
		}
		//find the next synonym in the string
		str = StringContainsWord(str + String::Length(replacement), synonym, false);
	}
}

bot_synonymlist_t* BotLoadSynonyms(const char* filename)
{
	char* ptr = NULL;
	int size = 0;
	bot_synonymlist_t* synlist = NULL;	//make compiler happy
	bot_synonymlist_t* syn = NULL;	//make compiler happy
	bot_synonym_t* synonym = NULL;	//make compiler happy
	//the synonyms are parsed in two phases
	for (int pass = 0; pass < 2; pass++)
	{
		if (pass && size)
		{
			ptr = (char*)Mem_ClearedAlloc(size);
		}

		if (GGameType & GAME_Quake3)
		{
			PC_SetBaseFolder(BOTFILESBASEFOLDER);
		}
		source_t* source = LoadSourceFile(filename);
		if (!source)
		{
			BotImport_Print(PRT_ERROR, "counldn't load %s\n", filename);
			return NULL;
		}

		unsigned int context = 0;
		int contextlevel = 0;
		unsigned int contextstack[32];
		synlist = NULL;	//list synonyms
		bot_synonymlist_t* lastsyn = NULL;	//last synonym in the list

		token_t token;
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
				}
				if (!PC_ExpectTokenString(source, "{"))
				{
					FreeSource(source);
					return NULL;
				}
			}
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
					}
					context &= ~contextstack[contextlevel];
				}
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
					}
					int numsynonyms = 0;
					bot_synonym_t* lastsynonym = NULL;
					while (1)
					{
						if (!PC_ExpectTokenString(source, "(") ||
							!PC_ExpectTokenType(source, TT_STRING, 0, &token))
						{
							FreeSource(source);
							return NULL;
						}
						StripDoubleQuotes(token.string);
						if (String::Length(token.string) <= 0)
						{
							SourceError(source, "empty string");
							FreeSource(source);
							return NULL;
						}
						size += sizeof(bot_synonym_t) + String::Length(token.string) + 1;
						if (pass)
						{
							synonym = (bot_synonym_t*)ptr;
							ptr += sizeof(bot_synonym_t);
							synonym->string = ptr;
							ptr += String::Length(token.string) + 1;
							String::Cpy(synonym->string, token.string);

							if (lastsynonym)
							{
								lastsynonym->next = synonym;
							}
							else
							{
								syn->firstsynonym = synonym;
							}
							lastsynonym = synonym;
						}
						numsynonyms++;
						if (!PC_ExpectTokenString(source, ",") ||
							!PC_ExpectTokenType(source, TT_NUMBER, 0, &token) ||
							!PC_ExpectTokenString(source, ")"))
						{
							FreeSource(source);
							return NULL;
						}
						if (pass)
						{
							synonym->weight = token.floatvalue;
							syn->totalweight += synonym->weight;
						}
						if (PC_CheckTokenString(source, "]"))
						{
							break;
						}
						if (!PC_ExpectTokenString(source, ","))
						{
							FreeSource(source);
							return NULL;
						}
					}
					if (numsynonyms < 2)
					{
						SourceError(source, "synonym must have at least two entries\n");
						FreeSource(source);
						return NULL;
					}
				}
				else
				{
					SourceError(source, "unexpected %s", token.string);
					FreeSource(source);
					return NULL;
				}
			}
		}

		FreeSource(source);

		if (contextlevel > 0)
		{
			SourceError(source, "missing }");
			return NULL;
		}
	}
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);
	return synlist;
}

// replace all the synonyms in the string
void BotReplaceSynonyms(char* string, unsigned int context)
{
	for (bot_synonymlist_t* syn = synonyms; syn; syn = syn->next)
	{
		if (!(syn->context & context))
		{
			continue;
		}
		for (bot_synonym_t* synonym = syn->firstsynonym->next; synonym; synonym = synonym->next)
		{
			StringReplaceWords(string, synonym->string, syn->firstsynonym->string);
		}
	}
}

void BotReplaceWeightedSynonyms(char* string, unsigned int context)
{
	for (bot_synonymlist_t* syn = synonyms; syn; syn = syn->next)
	{
		if (!(syn->context & context))
		{
			continue;
		}
		//choose a weighted random replacement synonym
		float weight = random() * syn->totalweight;
		if (!weight)
		{
			continue;
		}
		float curweight = 0;
		bot_synonym_t* replacement;
		for (replacement = syn->firstsynonym; replacement; replacement = replacement->next)
		{
			curweight += replacement->weight;
			if (weight < curweight)
			{
				break;
			}
		}
		if (!replacement)
		{
			continue;
		}
		//replace all synonyms with the replacement
		for (bot_synonym_t* synonym = syn->firstsynonym; synonym; synonym = synonym->next)
		{
			if (synonym == replacement)
			{
				continue;
			}
			StringReplaceWords(string, synonym->string, replacement->string);
		}
	}
}

void BotReplaceReplySynonyms(char* string, unsigned int context)
{
	for (char* str1 = string; *str1; )
	{
		//go to the start of the next word
		while (*str1 && *str1 <= ' ')
		{
			str1++;
		}
		if (!*str1)
		{
			break;
		}

		for (bot_synonymlist_t* syn = synonyms; syn; syn = syn->next)
		{
			if (!(syn->context & context))
			{
				continue;
			}
			bot_synonym_t* synonym;
			for (synonym = syn->firstsynonym->next; synonym; synonym = synonym->next)
			{
				char* str2 = synonym->string;
				//if the synonym is not at the front of the string continue
				str2 = StringContainsWord(str1, synonym->string, false);
				if (!str2 || str2 != str1)
				{
					continue;
				}

				char* replacement = syn->firstsynonym->string;
				//if the replacement IS in front of the string continue
				str2 = StringContainsWord(str1, replacement, false);
				if (str2 && str2 == str1)
				{
					continue;
				}

				memmove(str1 + String::Length(replacement), str1 + String::Length(synonym->string),
					String::Length(str1 + String::Length(synonym->string)) + 1);
				//append the synonum replacement
				Com_Memcpy(str1, replacement, String::Length(replacement));
				break;
			}
			//if a synonym has been replaced
			if (synonym)
			{
				break;
			}
		}
		//skip over this word
		while (*str1 && *str1 > ' ')
		{
			str1++;
		}
		if (!*str1)
		{
			break;
		}
	}
}

int BotLoadChatMessage(source_t* source, char* chatmessagestring)
{
	char* ptr = chatmessagestring;
	*ptr = 0;

	while (1)
	{
		token_t token;
		if (!PC_ExpectAnyToken(source, &token))
		{
			return false;
		}
		//fixed string
		if (token.type == TT_STRING)
		{
			StripDoubleQuotes(token.string);
			if (String::Length(ptr) + String::Length(token.string) + 1 > MAX_MESSAGE_SIZE)
			{
				SourceError(source, "chat message too long\n");
				return false;
			}
			String::Cat(ptr, MAX_MESSAGE_SIZE, token.string);
		}
		//variable string
		else if (token.type == TT_NUMBER && (token.subtype & TT_INTEGER))
		{
			if (String::Length(ptr) + 7 > MAX_MESSAGE_SIZE)
			{
				SourceError(source, "chat message too long\n");
				return false;
			}
			sprintf(&ptr[String::Length(ptr)], "%cv%d%c", ESCAPE_CHAR, token.intvalue, ESCAPE_CHAR);
		}
		//random string
		else if (token.type == TT_NAME)
		{
			if (String::Length(ptr) + 7 > MAX_MESSAGE_SIZE)
			{
				SourceError(source, "chat message too long\n");
				return false;
			}
			sprintf(&ptr[String::Length(ptr)], "%cr%s%c", ESCAPE_CHAR, token.string, ESCAPE_CHAR);
		}
		else
		{
			SourceError(source, "unknown message component %s\n", token.string);
			return false;
		}
		if (PC_CheckTokenString(source, ";"))
		{
			break;
		}
		if (!PC_ExpectTokenString(source, ","))
		{
			return false;
		}
	}
	return true;
}

bot_randomlist_t* BotLoadRandomStrings(const char* filename)
{
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif

	char* ptr = NULL;
	int size = 0;
	bot_randomlist_t* randomlist = NULL;
	bot_randomlist_t* random = NULL;
	//the synonyms are parsed in two phases
	for (int pass = 0; pass < 2; pass++)
	{
		if (pass && size)
		{
			ptr = (char*)Mem_ClearedAlloc(size);
		}

		if (GGameType & GAME_Quake3)
		{
			PC_SetBaseFolder(BOTFILESBASEFOLDER);
		}
		source_t* source = LoadSourceFile(filename);
		if (!source)
		{
			BotImport_Print(PRT_ERROR, "counldn't load %s\n", filename);
			return NULL;
		}

		randomlist = NULL;	//list
		bot_randomlist_t* lastrandom = NULL;	//last

		token_t token;
		while (PC_ReadToken(source, &token))
		{
			if (token.type != TT_NAME)
			{
				SourceError(source, "unknown random %s", token.string);
				FreeSource(source);
				return NULL;
			}
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
			}
			if (!PC_ExpectTokenString(source, "=") ||
				!PC_ExpectTokenString(source, "{"))
			{
				FreeSource(source);
				return NULL;
			}
			while (!PC_CheckTokenString(source, "}"))
			{
				char chatmessagestring[MAX_MESSAGE_SIZE];
				if (!BotLoadChatMessage(source, chatmessagestring))
				{
					FreeSource(source);
					return NULL;
				}
				size += sizeof(bot_randomstring_t) + String::Length(chatmessagestring) + 1;
				if (pass)
				{
					bot_randomstring_t* randomstring = (bot_randomstring_t*)ptr;
					ptr += sizeof(bot_randomstring_t);
					randomstring->string = ptr;
					ptr += String::Length(chatmessagestring) + 1;
					String::Cpy(randomstring->string, chatmessagestring);

					random->numstrings++;
					randomstring->next = random->firstrandomstring;
					random->firstrandomstring = randomstring;
				}
			}
		}
		//free the source after one pass
		FreeSource(source);
	}
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);

#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "random strings %d msec\n", Sys_Milliseconds() - starttime);
#endif
	return randomlist;
}

char* RandomString(const char* name)
{
	bot_randomlist_t* random;
	bot_randomstring_t* rs;
	int i;

	for (random = randomstrings; random; random = random->next)
	{
		if (!String::Cmp(random->string, name))
		{
			i = random() * random->numstrings;
			for (rs = random->firstrandomstring; rs; rs = rs->next)
			{
				if (--i < 0)
				{
					break;
				}
			}
			if (rs)
			{
				return rs->string;
			}
		}
	}
	return NULL;
}

void BotFreeMatchPieces(bot_matchpiece_t* matchpieces)
{
	bot_matchpiece_t* nextmp;
	for (bot_matchpiece_t* mp = matchpieces; mp; mp = nextmp)
	{
		nextmp = mp->next;
		if (mp->type == MT_STRING)
		{
			bot_matchstring_t* nextms;
			for (bot_matchstring_t* ms = mp->firststring; ms; ms = nextms)
			{
				nextms = ms->next;
				Mem_Free(ms);
			}
		}
		Mem_Free(mp);
	}
}

bot_matchpiece_t* BotLoadMatchPieces(source_t* source, const char* endtoken)
{
	bot_matchpiece_t* firstpiece = NULL;
	bot_matchpiece_t* lastpiece = NULL;

	bool lastwasvariable = false;

	token_t token;
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
			}
			if (lastwasvariable)
			{
				SourceError(source, "not allowed to have adjacent variables\n");
				FreeSource(source);
				BotFreeMatchPieces(firstpiece);
				return NULL;
			}
			lastwasvariable = true;

			bot_matchpiece_t* matchpiece = (bot_matchpiece_t*)Mem_ClearedAlloc(sizeof(bot_matchpiece_t));
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
		}
		else if (token.type == TT_STRING)
		{
			bot_matchpiece_t* matchpiece = (bot_matchpiece_t*)Mem_ClearedAlloc(sizeof(bot_matchpiece_t));
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

			bot_matchstring_t* lastmatchstring = NULL;
			bool emptystring = false;

			do
			{
				if (matchpiece->firststring)
				{
					if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
					{
						FreeSource(source);
						BotFreeMatchPieces(firstpiece);
						return NULL;
					}
				}
				StripDoubleQuotes(token.string);
				bot_matchstring_t* matchstring = (bot_matchstring_t*)Mem_ClearedAlloc(sizeof(bot_matchstring_t) + String::Length(token.string) + 1);
				matchstring->string = (char*)matchstring + sizeof(bot_matchstring_t);
				String::Cpy(matchstring->string, token.string);
				if (!String::Length(token.string))
				{
					emptystring = true;
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
				lastwasvariable = false;
			}
		}
		else
		{
			SourceError(source, "invalid token %s\n", token.string);
			FreeSource(source);
			BotFreeMatchPieces(firstpiece);
			return NULL;
		}
		if (PC_CheckTokenString(source, endtoken))
		{
			break;
		}
		if (!PC_ExpectTokenString(source, ","))
		{
			FreeSource(source);
			BotFreeMatchPieces(firstpiece);
			return NULL;
		}
	}
	return firstpiece;
}

void BotFreeMatchTemplates(bot_matchtemplate_t* mt)
{
	bot_matchtemplate_t* nextmt;
	for (; mt; mt = nextmt)
	{
		nextmt = mt->next;
		BotFreeMatchPieces(mt->first);
		Mem_Free(mt);
	}
}

bot_matchtemplate_t* BotLoadMatchTemplates(const char* matchfile)
{
	if (GGameType & GAME_Quake3)
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	source_t* source = LoadSourceFile(matchfile);
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", matchfile);
		return NULL;
	}

	bot_matchtemplate_t* matches = NULL;	//list with matches
	bot_matchtemplate_t* lastmatch = NULL;	//last match in the list

	token_t token;
	while (PC_ReadToken(source, &token))
	{
		if (token.type != TT_NUMBER || !(token.subtype & TT_INTEGER))
		{
			SourceError(source, "expected integer, found %s\n", token.string);
			BotFreeMatchTemplates(matches);
			FreeSource(source);
			return NULL;
		}
		//the context
		unsigned int context = token.intvalue;

		if (!PC_ExpectTokenString(source, "{"))
		{
			BotFreeMatchTemplates(matches);
			FreeSource(source);
			return NULL;
		}

		while (PC_ReadToken(source, &token))
		{
			if (!String::Cmp(token.string, "}"))
			{
				break;
			}

			PC_UnreadLastToken(source);

			bot_matchtemplate_t* matchtemplate = (bot_matchtemplate_t*)Mem_ClearedAlloc(sizeof(bot_matchtemplate_t));
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
			}
			//read the match type
			if (!PC_ExpectTokenString(source, "(") ||
				!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return NULL;
			}
			matchtemplate->type = token.intvalue;
			//read the match subtype
			if (!PC_ExpectTokenString(source, ",") ||
				!PC_ExpectTokenType(source, TT_NUMBER, TT_INTEGER, &token))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return NULL;
			}
			matchtemplate->subtype = token.intvalue;
			//read trailing punctuations
			if (!PC_ExpectTokenString(source, ")") ||
				!PC_ExpectTokenString(source, ";"))
			{
				BotFreeMatchTemplates(matches);
				FreeSource(source);
				return NULL;
			}
		}
	}
	//free the source
	FreeSource(source);
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", matchfile);
	return matches;
}
