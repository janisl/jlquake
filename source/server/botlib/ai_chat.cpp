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

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "local.h"

//escape character
#define ESCAPE_CHAR             0x01	//'_'
//
// "hi ", people, " ", 0, " entered the game"
//becomes:
// "hi _rpeople_ _v0_ entered the game"
//

//match piece types
#define MT_VARIABLE                 1		//variable match piece
#define MT_STRING                   2		//string match piece
//reply chat key flags
#define RCKFL_AND                   1		//key must be present
#define RCKFL_NOT                   2		//key must be absent
#define RCKFL_NAME                  4		//name of bot must be present
#define RCKFL_STRING                8		//key is a string
#define RCKFL_VARIABLES             16		//key is a match template
#define RCKFL_BOTNAMES              32		//key is a series of botnames
#define RCKFL_GENDERFEMALE          64		//bot must be female
#define RCKFL_GENDERMALE            128		//bot must be male
#define RCKFL_GENDERLESS            256		//bot must be genderless
//time to ignore a chat message after using it
#define CHATMESSAGE_RECENTTIME  20

#define MAX_MESSAGE_SIZE        256

//the actuall chat messages
struct bot_chatmessage_t
{
	char* chatmessage;					//chat message string
	float time;							//last time used
	bot_chatmessage_t* next;		//next chat message in a list
};

//bot chat type with chat lines
struct bot_chattype_t
{
	char name[MAX_CHATTYPE_NAME];
	int numchatmessages;
	bot_chatmessage_t* firstchatmessage;
	bot_chattype_t* next;
};

//bot chat lines
struct bot_chat_t
{
	bot_chattype_t* types;
};

struct bot_consolemessage_t
{
	int handle;
	float time;							//message time
	int type;							//message type
	char message[MAX_MESSAGE_SIZE];		//message
	bot_consolemessage_t* prev, * next;	//prev and next in list
};

//chat state of a bot
struct bot_chatstate_t
{
	int gender;											//0=it, 1=female, 2=male
	int client;											//client number
	char name[32];										//name of the bot
	char chatmessage[MAX_MESSAGE_SIZE];
	int handle;
	//the console messages visible to the bot
	bot_consolemessage_t* firstmessage;			//first message is the first typed message
	bot_consolemessage_t* lastmessage;			//last message is the last typed message, bottom of console
	//number of console messages stored in the state
	int numconsolemessages;
	//the bot chat lines
	bot_chat_t* chat;
};

//random string
struct bot_randomstring_t
{
	char* string;
	bot_randomstring_t* next;
};

//list with random strings
struct bot_randomlist_t
{
	char* string;
	int numstrings;
	bot_randomstring_t* firstrandomstring;
	bot_randomlist_t* next;
};

//synonym
struct bot_synonym_t
{
	char* string;
	float weight;
	bot_synonym_t* next;
};

//list with synonyms
struct bot_synonymlist_t
{
	unsigned int context;
	float totalweight;
	bot_synonym_t* firstsynonym;
	bot_synonymlist_t* next;
};

//fixed match string
struct bot_matchstring_t
{
	char* string;
	bot_matchstring_t* next;
};

//piece of a match template
struct bot_matchpiece_t
{
	int type;
	bot_matchstring_t* firststring;
	int variable;
	bot_matchpiece_t* next;
};

//match template
struct bot_matchtemplate_t
{
	unsigned int context;
	int type;
	int subtype;
	bot_matchpiece_t* first;
	bot_matchtemplate_t* next;
};

//reply chat key
struct bot_replychatkey_t
{
	int flags;
	char* string;
	bot_matchpiece_t* match;
	bot_replychatkey_t* next;
};

//reply chat
struct bot_replychat_t
{
	bot_replychatkey_t* keys;
	float priority;
	int numchatmessages;
	bot_chatmessage_t* firstchatmessage;
	bot_replychat_t* next;
};

//string list
struct bot_stringlist_t
{
	char* string;
	bot_stringlist_t* next;
};

struct bot_ichatdata_t
{
	bot_chat_t* chat;
	char filename[MAX_QPATH];
	char chatname[MAX_QPATH];
};

struct bot_matchvariable_t
{
	const char* ptr;
	int length;
};

struct bot_match_t
{
	char string[MAX_MESSAGE_SIZE];
	int type;
	int subtype;
	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
};

static bot_chatstate_t* botchatstates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
//console message heap
static bot_consolemessage_t* consolemessageheap = NULL;
static bot_consolemessage_t* freeconsolemessages = NULL;
//list with match strings
static bot_matchtemplate_t* matchtemplates = NULL;
//list with synonyms
static bot_synonymlist_t* synonyms = NULL;
//list with random strings
static bot_randomlist_t* randomstrings = NULL;
//reply chats
static bot_replychat_t* replychats = NULL;
static bot_ichatdata_t* ichatdata[MAX_BOTLIB_CLIENTS_ARRAY];

static bot_chatstate_t* BotChatStateFromHandle(int handle)
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
static void InitConsoleMessageHeap()
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
static void FreeConsoleMessage(bot_consolemessage_t* message)
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

static void BotRemoveTildes(char* message)
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

static char* StringContainsWord(char* str1, const char* str2, bool casesensitive)
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

static bot_synonymlist_t* BotLoadSynonyms(const char* filename)
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

static void BotReplaceWeightedSynonyms(char* string, unsigned int context)
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

static void BotReplaceReplySynonyms(char* string, unsigned int context)
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
				const char* str2 = synonym->string;
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

static int BotLoadChatMessage(source_t* source, char* chatmessagestring)
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

static bot_randomlist_t* BotLoadRandomStrings(const char* filename)
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

static char* RandomString(const char* name)
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

static void BotFreeMatchPieces(bot_matchpiece_t* matchpieces)
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

static bot_matchpiece_t* BotLoadMatchPieces(source_t* source, const char* endtoken)
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

static void BotFreeMatchTemplates(bot_matchtemplate_t* mt)
{
	bot_matchtemplate_t* nextmt;
	for (; mt; mt = nextmt)
	{
		nextmt = mt->next;
		BotFreeMatchPieces(mt->first);
		Mem_Free(mt);
	}
}

static bot_matchtemplate_t* BotLoadMatchTemplates(const char* matchfile)
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

static bool StringsMatch(bot_matchpiece_t* pieces, bot_match_t* match)
{
	//no last variable
	int lastvariable = -1;
	//pointer to the string to compare the match string with
	char* strptr = match->string;
	//compare the string with the current match string
	bot_matchpiece_t* mp;
	for (mp = pieces; mp; mp = mp->next)
	{
		//if it is a piece of string
		if (mp->type == MT_STRING)
		{
			char* newstrptr = NULL;
			bot_matchstring_t* ms;
			for (ms = mp->firststring; ms; ms = ms->next)
			{
				if (!String::Length(ms->string))
				{
					newstrptr = strptr;
					break;
				}
				int index = StringContains(strptr, ms->string, false);
				if (index >= 0)
				{
					newstrptr = strptr + index;
					if (lastvariable >= 0)
					{
						match->variables[lastvariable].length =
							newstrptr - match->variables[lastvariable].ptr;
						lastvariable = -1;
						break;
					}
					else if (index == 0)
					{
						break;
					}
					newstrptr = NULL;
				}
			}
			if (!newstrptr)
			{
				return false;
			}
			strptr = newstrptr + String::Length(ms->string);
		}
		//if it is a variable piece of string
		else if (mp->type == MT_VARIABLE)
		{
			match->variables[mp->variable].ptr = strptr;
			lastvariable = mp->variable;
		}
	}
	//if a match was found
	if (!mp && (lastvariable >= 0 || !String::Length(strptr)))
	{
		//if the last piece was a variable string
		if (lastvariable >= 0)
		{
			match->variables[lastvariable].length =
				String::Length(match->variables[lastvariable].ptr);
		}
		return true;
	}
	return false;
}

static bool BotFindMatch(const char* str, bot_match_t* match, unsigned int context)
{
	Com_Memset(match, 0, sizeof(*match));
	String::NCpyZ(match->string, str, sizeof(match->string));
	//remove any trailing enters
	while (String::Length(match->string) &&
		match->string[String::Length(match->string) - 1] == '\n')
	{
		match->string[String::Length(match->string) - 1] = '\0';
	}
	//compare the string with all the match strings
	for (bot_matchtemplate_t* ms = matchtemplates; ms; ms = ms->next)
	{
		if (!(ms->context & context))
		{
			continue;
		}
		//reset the match variable offsets
		for (int i = 0; i < MAX_MATCHVARIABLES; i++)
		{
			match->variables[i].ptr = NULL;
		}

		if (StringsMatch(ms->first, match))
		{
			match->type = ms->type;
			match->subtype = ms->subtype;
			return true;
		}
	}
	return false;
}

static void MatchIntToQ3(bot_match_t* src, bot_match_q3_t* dst)
{
	String::NCpyZ(dst->string, src->string, sizeof(dst->string));
	dst->type = src->type;
	dst->subtype = src->subtype;
	for (int i = 0; i < MAX_MATCHVARIABLES; i++)
	{
		dst->variables[i].offset = src->variables[i].ptr ? src->variables[i].ptr - src->string : -1;
		dst->variables[i].length = src->variables[i].length;
	}
}

static void MatchIntToWolf(bot_match_t* src, bot_match_wolf_t* dst)
{
	String::NCpyZ(dst->string, src->string, sizeof(dst->string));
	dst->type = src->type;
	dst->subtype = src->subtype;
	for (int i = 0; i < MAX_MATCHVARIABLES; i++)
	{
		dst->variables[i].ptr = src->variables[i].ptr ? dst->string + (src->variables[i].ptr - src->string) : NULL;
		dst->variables[i].length = src->variables[i].length;
	}
}

bool BotFindMatchQ3(const char* str, bot_match_q3_t* match, unsigned int context)
{
	bot_match_t intMatch;
	bool ret = BotFindMatch(str, &intMatch, context);
	MatchIntToQ3(&intMatch, match);
	return ret;
}

bool BotFindMatchWolf(const char* str, bot_match_wolf_t* match, unsigned int context)
{
	bot_match_t intMatch;
	bool ret = BotFindMatch(str, &intMatch, context);
	MatchIntToWolf(&intMatch, match);
	return ret;
}

static void BotMatchVariable(bot_match_t* match, int variable, char* buf, int size)
{
	if (variable < 0 || variable >= MAX_MATCHVARIABLES)
	{
		BotImport_Print(PRT_FATAL, "BotMatchVariableQ3: variable out of range\n");
		String::Cpy(buf, "");
		return;
	}

	if (match->variables[variable].ptr)
	{
		if (match->variables[variable].length < size)
		{
			size = match->variables[variable].length + 1;
		}
		String::NCpyZ(buf, match->variables[variable].ptr, size);
	}
	else
	{
		String::Cpy(buf, "");
	}
	return;
}

static void MatchQ3ToInt(bot_match_q3_t* src, bot_match_t* dst)
{
	String::NCpyZ(dst->string, src->string, sizeof(dst->string));
	dst->type = src->type;
	dst->subtype = src->subtype;
	for (int i = 0; i < MAX_MATCHVARIABLES; i++)
	{
		dst->variables[i].ptr = src->variables[i].offset >= 0 ? dst->string + src->variables[i].offset: NULL;
		dst->variables[i].length = src->variables[i].length;
	}
}

static void MatchWolfToInt(bot_match_wolf_t* src, bot_match_t* dst)
{
	String::NCpyZ(dst->string, src->string, sizeof(dst->string));
	dst->type = src->type;
	dst->subtype = src->subtype;
	for (int i = 0; i < MAX_MATCHVARIABLES; i++)
	{
		dst->variables[i].ptr = src->variables[i].ptr ? dst->string + (src->variables[i].ptr - src->string) : NULL;
		dst->variables[i].length = src->variables[i].length;
	}
}

void BotMatchVariableQ3(bot_match_q3_t* match, int variable, char* buf, int size)
{
	bot_match_t intMatch;
	MatchQ3ToInt(match, &intMatch);
	BotMatchVariable(&intMatch, variable, buf, size);
}

void BotMatchVariableWolf(bot_match_wolf_t* match, int variable, char* buf, int size)
{
	bot_match_t intMatch;
	MatchWolfToInt(match, &intMatch);
	BotMatchVariable(&intMatch, variable, buf, size);
}

static bot_stringlist_t* BotFindStringInList(bot_stringlist_t* list, const char* string)
{
	for (bot_stringlist_t* s = list; s; s = s->next)
	{
		if (!String::Cmp(s->string, string))
		{
			return s;
		}
	}
	return NULL;
}

static bot_stringlist_t* BotCheckChatMessageIntegrety(const char* message, bot_stringlist_t* stringlist)
{
	int i;
	char temp[MAX_MESSAGE_SIZE];

	const char* msgptr = message;

	while (*msgptr)
	{
		if (*msgptr == ESCAPE_CHAR)
		{
			msgptr++;
			switch (*msgptr)
			{
			case 'v':	//variable
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
			case 'r':	//random
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
						bot_stringlist_t* s = (bot_stringlist_t*)Mem_ClearedAlloc(sizeof(bot_stringlist_t) + String::Length(temp) + 1);
						s->string = (char*)s + sizeof(bot_stringlist_t);
						String::Cpy(s->string, temp);
						s->next = stringlist;
						stringlist = s;
					}
				}
				break;
			default:
				BotImport_Print(PRT_FATAL, "BotCheckChatMessageIntegrety: message \"%s\" invalid escape char\n", message);
				break;
			}
		}
		else
		{
			msgptr++;
		}
	}
	return stringlist;
}

static void BotCheckInitialChatIntegrety(bot_chat_t* chat)
{
	bot_stringlist_t* stringlist = NULL;
	for (bot_chattype_t* t = chat->types; t; t = t->next)
	{
		for (bot_chatmessage_t* cm = t->firstchatmessage; cm; cm = cm->next)
		{
			stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
		}
	}
	bot_stringlist_t* nexts;
	for (bot_stringlist_t* s = stringlist; s; s = nexts)
	{
		nexts = s->next;
		Mem_Free(s);
	}
}

static void BotCheckReplyChatIntegrety(bot_replychat_t* replychat)
{
	bot_stringlist_t* stringlist = NULL;
	for (bot_replychat_t* rp = replychat; rp; rp = rp->next)
	{
		for (bot_chatmessage_t* cm = rp->firstchatmessage; cm; cm = cm->next)
		{
			stringlist = BotCheckChatMessageIntegrety(cm->chatmessage, stringlist);
		}
	}
	bot_stringlist_t* nexts;
	for (bot_stringlist_t* s = stringlist; s; s = nexts)
	{
		nexts = s->next;
		Mem_Free(s);
	}
}

static void BotFreeReplyChat(bot_replychat_t* replychat)
{
	bot_replychat_t* nextrp;
	for (bot_replychat_t* rp = replychat; rp; rp = nextrp)
	{
		nextrp = rp->next;
		bot_replychatkey_t* nextkey;
		for (bot_replychatkey_t* key = rp->keys; key; key = nextkey)
		{
			nextkey = key->next;
			if (key->match)
			{
				BotFreeMatchPieces(key->match);
			}
			if (key->string)
			{
				Mem_Free(key->string);
			}
			Mem_Free(key);
		}
		bot_chatmessage_t* nextcm;
		for (bot_chatmessage_t* cm = rp->firstchatmessage; cm; cm = nextcm)
		{
			nextcm = cm->next;
			Mem_Free(cm);
		}
		Mem_Free(rp);
	}
}

static void BotCheckValidReplyChatKeySet(source_t* source, bot_replychatkey_t* keys)
{
	bool allprefixed = true;
	bool hasvariableskey =  false;
	bool hasstringkey = false;
	for (bot_replychatkey_t* key = keys; key; key = key->next)
	{
		if (!(key->flags & (RCKFL_AND | RCKFL_NOT)))
		{
			allprefixed = false;
			if (key->flags & RCKFL_VARIABLES)
			{
				for (bot_matchpiece_t* m = key->match; m; m = m->next)
				{
					if (m->type == MT_VARIABLE)
					{
						hasvariableskey = true;
					}
				}
			}
			else if (key->flags & RCKFL_STRING)
			{
				hasstringkey = true;
			}
		}
		else if ((key->flags & RCKFL_AND) && (key->flags & RCKFL_STRING))
		{
			for (bot_replychatkey_t* key2 = keys; key2; key2 = key2->next)
			{
				if (key2 == key)
				{
					continue;
				}
				if (key2->flags & RCKFL_NOT)
				{
					continue;
				}
				if (key2->flags & RCKFL_VARIABLES)
				{
					bot_matchpiece_t* m;
					for (m = key2->match; m; m = m->next)
					{
						if (m->type == MT_STRING)
						{
							bot_matchstring_t* ms;
							for (ms = m->firststring; ms; ms = ms->next)
							{
								if (StringContains(ms->string, key->string, false) != -1)
								{
									break;
								}
							}
							if (ms)
							{
								break;
							}
						}
						else if (m->type == MT_VARIABLE)
						{
							break;
						}
					}
					if (!m)
					{
						SourceWarning(source, "one of the match templates does not "
											  "leave space for the key %s with the & prefix", key->string);
					}
				}
			}
		}
		if ((key->flags & RCKFL_NOT) && (key->flags & RCKFL_STRING))
		{
			for (bot_replychatkey_t* key2 = keys; key2; key2 = key2->next)
			{
				if (key2 == key)
				{
					continue;
				}
				if (key2->flags & RCKFL_NOT)
				{
					continue;
				}
				if (key2->flags & RCKFL_STRING)
				{
					if (StringContains(key2->string, key->string, false) != -1)
					{
						SourceWarning(source, "the key %s with prefix ! is inside the key %s", key->string, key2->string);
					}
				}
				else if (key2->flags & RCKFL_VARIABLES)
				{
					for (bot_matchpiece_t* m = key2->match; m; m = m->next)
					{
						if (m->type == MT_STRING)
						{
							for (bot_matchstring_t* ms = m->firststring; ms; ms = ms->next)
							{
								if (StringContains(ms->string, key->string, false) != -1)
								{
									SourceWarning(source, "the key %s with prefix ! is inside "
														  "the match template string %s", key->string, ms->string);
								}
							}
						}
					}
				}
			}
		}
	}
	if (allprefixed)
	{
		SourceWarning(source, "all keys have a & or ! prefix");
	}
	if (hasvariableskey && hasstringkey)
	{
		SourceWarning(source, "variables from the match template(s) could be "
							  "invalid when outputting one of the chat messages");
	}
}

static bot_replychat_t* BotLoadReplyChat(const char* filename)
{
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

	bot_replychat_t* replychatlist = NULL;

	token_t token;
	while (PC_ReadToken(source, &token))
	{
		if (String::Cmp(token.string, "["))
		{
			SourceError(source, "expected [, found %s", token.string);
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return NULL;
		}

		bot_replychat_t* replychat = (bot_replychat_t*)Mem_ClearedAlloc(sizeof(bot_replychat_t));
		replychat->keys = NULL;
		replychat->next = replychatlist;
		replychatlist = replychat;
		//read the keys, there must be at least one key
		do
		{
			//allocate a key
			bot_replychatkey_t* key = (bot_replychatkey_t*)Mem_ClearedAlloc(sizeof(bot_replychatkey_t));
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
			else if (PC_CheckTokenString(source, "("))	//match key
			{
				key->flags |= RCKFL_VARIABLES;
				key->match = BotLoadMatchPieces(source, ")");
				if (!key->match)
				{
					BotFreeReplyChat(replychatlist);
					return NULL;
				}
			}
			else if (PC_CheckTokenString(source, "<"))	//bot names
			{
				key->flags |= RCKFL_BOTNAMES;
				char namebuffer[MAX_MESSAGE_SIZE];
				String::Cpy(namebuffer, "");
				do
				{
					if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
					{
						BotFreeReplyChat(replychatlist);
						FreeSource(source);
						return NULL;
					}
					StripDoubleQuotes(token.string);
					if (String::Length(namebuffer))
					{
						String::Cat(namebuffer, sizeof(namebuffer), "\\");
					}
					String::Cat(namebuffer, sizeof(namebuffer), token.string);
				}
				while (PC_CheckTokenString(source, ","));
				if (!PC_ExpectTokenString(source, ">"))
				{
					BotFreeReplyChat(replychatlist);
					FreeSource(source);
					return NULL;
				}
				key->string = (char*)Mem_ClearedAlloc(String::Length(namebuffer) + 1);
				String::Cpy(key->string, namebuffer);
			}
			else//normal string key
			{
				key->flags |= RCKFL_STRING;
				if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
				{
					BotFreeReplyChat(replychatlist);
					FreeSource(source);
					return NULL;
				}
				StripDoubleQuotes(token.string);
				key->string = (char*)Mem_ClearedAlloc(String::Length(token.string) + 1);
				String::Cpy(key->string, token.string);
			}

			PC_CheckTokenString(source, ",");
		}
		while (!PC_CheckTokenString(source, "]"));

		BotCheckValidReplyChatKeySet(source, replychat->keys);
		//read the = sign and the priority
		if (!PC_ExpectTokenString(source, "=") ||
			!PC_ExpectTokenType(source, TT_NUMBER, 0, &token))
		{
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return NULL;
		}
		replychat->priority = token.floatvalue;
		//read the leading {
		if (!PC_ExpectTokenString(source, "{"))
		{
			BotFreeReplyChat(replychatlist);
			FreeSource(source);
			return NULL;
		}
		replychat->numchatmessages = 0;
		//while the trailing } is not found
		while (!PC_CheckTokenString(source, "}"))
		{
			char chatmessagestring[MAX_MESSAGE_SIZE];
			if (!BotLoadChatMessage(source, chatmessagestring))
			{
				BotFreeReplyChat(replychatlist);
				FreeSource(source);
				return NULL;
			}
			bot_chatmessage_t* chatmessage = (bot_chatmessage_t*)Mem_ClearedAlloc(sizeof(bot_chatmessage_t) + String::Length(chatmessagestring) + 1);
			chatmessage->chatmessage = (char*)chatmessage + sizeof(bot_chatmessage_t);
			String::Cpy(chatmessage->chatmessage, chatmessagestring);
			chatmessage->time = -2 * CHATMESSAGE_RECENTTIME;
			chatmessage->next = replychat->firstchatmessage;
			//add the chat message to the reply chat
			replychat->firstchatmessage = chatmessage;
			replychat->numchatmessages++;
		}
	}
	FreeSource(source);
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", filename);

	if (bot_developer)
	{
		BotCheckReplyChatIntegrety(replychatlist);
	}

	if (!replychatlist)
	{
		BotImport_Print(PRT_MESSAGE, "no rchats\n");
	}

	return replychatlist;
}

static bot_chat_t* BotLoadInitialChat(const char* chatfile, const char* chatname)
{
	char* ptr = NULL;
	bot_chat_t* chat = NULL;
	bot_chattype_t* chattype = NULL;
	bot_chatmessage_t* chatmessage = NULL;
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif

	int size = 0;
	bool foundchat = false;
	//a bot chat is parsed in two phases
	for (int pass = 0; pass < 2; pass++)
	{
		//allocate memory
		if (pass && size)
		{
			ptr = (char*)Mem_ClearedAlloc(size);
		}
		//load the source file
		if (GGameType & GAME_Quake3)
		{
			PC_SetBaseFolder(BOTFILESBASEFOLDER);
		}
		source_t* source = LoadSourceFile(chatfile);
		if (!source)
		{
			BotImport_Print(PRT_ERROR, "counldn't load %s\n", chatfile);
			return NULL;
		}
		//chat structure
		if (pass)
		{
			chat = (bot_chat_t*)ptr;
			ptr += sizeof(bot_chat_t);
		}
		size = sizeof(bot_chat_t);

		token_t token;
		while (PC_ReadToken(source, &token))
		{
			if (!String::Cmp(token.string, "chat"))
			{
				if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
				{
					FreeSource(source);
					return NULL;
				}
				StripDoubleQuotes(token.string);
				//after the chat name we expect a opening brace
				if (!PC_ExpectTokenString(source, "{"))
				{
					FreeSource(source);
					return NULL;
				}
				//if the chat name is found
				if (!String::ICmp(token.string, chatname))
				{
					foundchat = true;
					//read the chat types
					while (1)
					{
						if (!PC_ExpectAnyToken(source, &token))
						{
							FreeSource(source);
							return NULL;
						}
						if (!String::Cmp(token.string, "}"))
						{
							break;
						}
						if (String::Cmp(token.string, "type"))
						{
							SourceError(source, "expected type found %s\n", token.string);
							FreeSource(source);
							return NULL;
						}
						//expect the chat type name
						if (!PC_ExpectTokenType(source, TT_STRING, 0, &token) ||
							!PC_ExpectTokenString(source, "{"))
						{
							FreeSource(source);
							return NULL;
						}
						StripDoubleQuotes(token.string);
						if (pass)
						{
							chattype = (bot_chattype_t*)ptr;
							String::NCpy(chattype->name, token.string, MAX_CHATTYPE_NAME);
							chattype->firstchatmessage = NULL;
							//add the chat type to the chat
							chattype->next = chat->types;
							chat->types = chattype;

							ptr += sizeof(bot_chattype_t);
						}
						size += sizeof(bot_chattype_t);
						//read the chat messages
						while (!PC_CheckTokenString(source, "}"))
						{
							char chatmessagestring[MAX_MESSAGE_SIZE];
							if (!BotLoadChatMessage(source, chatmessagestring))
							{
								FreeSource(source);
								return NULL;
							}
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
							}
							size += sizeof(bot_chatmessage_t) + String::Length(chatmessagestring) + 1;
						}
					}
				}
				else//skip the bot chat
				{
					int indent = 1;
					while (indent)
					{
						if (!PC_ExpectAnyToken(source, &token))
						{
							FreeSource(source);
							return NULL;
						}
						if (!String::Cmp(token.string, "{"))
						{
							indent++;
						}
						else if (!String::Cmp(token.string, "}"))
						{
							indent--;
						}
					}
				}
			}
			else
			{
				SourceError(source, "unknown definition %s\n", token.string);
				FreeSource(source);
				return NULL;
			}
		}
		//free the source
		FreeSource(source);
		//if the requested character is not found
		if (!foundchat)
		{
			BotImport_Print(PRT_ERROR, "couldn't find chat %s in %s\n", chatname, chatfile);
			return NULL;
		}
	}

	BotImport_Print(PRT_MESSAGE, "loaded %s from %s\n", chatname, chatfile);

	if (bot_developer)
	{
		BotCheckInitialChatIntegrety(chat);
	}
#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "initial chats loaded in %d msec\n", Sys_Milliseconds() - starttime);
#endif
	//character was read succesfully
	return chat;
}

static void BotFreeChatFile(int chatstate)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	if (cs->chat)
	{
		Mem_Free(cs->chat);
	}
	cs->chat = NULL;
}

int BotLoadChatFile(int chatstate, const char* chatfile, const char* chatname)
{
	bot_chatstate_t* cs;
	int n, avail = 0;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADICHAT : WOLFBLERR_CANNOTLOADICHAT;
	}
	BotFreeChatFile(chatstate);

	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		avail = -1;
		for (n = 0; n < MAX_BOTLIB_CLIENTS; n++)
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
			return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADICHAT : WOLFBLERR_CANNOTLOADICHAT;
		}
	}

	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	cs->chat = BotLoadInitialChat(chatfile, chatname);
	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder("");
	}
	if (!cs->chat)
	{
		BotImport_Print(PRT_FATAL, "couldn't load chat %s from %s\n", chatname, chatfile);
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADICHAT : WOLFBLERR_CANNOTLOADICHAT;
	}	//end if
	if (!LibVarGetValue("bot_reloadcharacters"))
	{
		ichatdata[avail] = (bot_ichatdata_t*)Mem_ClearedAlloc(sizeof(bot_ichatdata_t));
		ichatdata[avail]->chat = cs->chat;
		String::NCpyZ(ichatdata[avail]->chatname, chatname, sizeof(ichatdata[avail]->chatname));
		String::NCpyZ(ichatdata[avail]->filename, chatfile, sizeof(ichatdata[avail]->filename));
	}	//end if

	return BLERR_NOERROR;
}

static bool BotExpandChatMessage(char* outmessage, const char* message, unsigned mcontext,
	bot_matchvariable_t* variables, unsigned vcontext, bool reply)
{
	bool expansion = false;
	const char* msgptr = message;
	char* outputbuf = outmessage;
	int len = 0;

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
				int num = 0;
				while (*msgptr && *msgptr != ESCAPE_CHAR)
				{
					num = num * 10 + (*msgptr++) - '0';
				}
				//step over the trailing escape char
				if (*msgptr)
				{
					msgptr++;
				}
				if (num > MAX_MATCHVARIABLES)
				{
					BotImport_Print(PRT_ERROR, "BotConstructChat: message %s variable %d out of range\n", message, num);
					return false;
				}
				const char* ptr = variables[num].ptr;
				if (ptr)
				{
					char temp[MAX_MESSAGE_SIZE];
					int i;
					for (i = 0; i < variables[num].length; i++)
					{
						temp[i] = ptr[i];
					}
					temp[i] = 0;
					//if it's a reply message
					if (reply)
					{
						//replace the reply synonyms in the variables
						BotReplaceReplySynonyms(temp, vcontext);
					}
					else
					{
						//replace synonyms in the variable context
						BotReplaceSynonyms(temp, vcontext);
					}

					if (len + String::Length(temp) >= MAX_MESSAGE_SIZE)
					{
						BotImport_Print(PRT_ERROR, "BotConstructChat: message %s too long\n", message);
						return false;
					}
					String::Cpy(&outputbuf[len], temp);
					len += String::Length(temp);
				}
				break;
			}
			case 'r':	//random
			{
				msgptr++;
				char temp[MAX_MESSAGE_SIZE];
				int i;
				for (i = 0; (*msgptr && *msgptr != ESCAPE_CHAR); i++)
				{
					temp[i] = *msgptr++;
				}
				temp[i] = '\0';
				//step over the trailing escape char
				if (*msgptr)
				{
					msgptr++;
				}
				//find the random keyword
				char* ptr = RandomString(temp);
				if (!ptr)
				{
					BotImport_Print(PRT_ERROR, "BotConstructChat: unknown random string %s\n", temp);
					return false;
				}
				if (len + String::Length(ptr) >= MAX_MESSAGE_SIZE)
				{
					BotImport_Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
					return false;
				}
				String::Cpy(&outputbuf[len], ptr);
				len += String::Length(ptr);
				expansion = true;
				break;
			}
			default:
				BotImport_Print(PRT_FATAL, "BotConstructChat: message \"%s\" invalid escape char\n", message);
				break;
			}
		}
		else
		{
			outputbuf[len++] = *msgptr++;
			if (len >= MAX_MESSAGE_SIZE)
			{
				BotImport_Print(PRT_ERROR, "BotConstructChat: message \"%s\" too long\n", message);
				break;
			}
		}
	}
	outputbuf[len] = '\0';
	//replace synonyms weighted in the message context
	BotReplaceWeightedSynonyms(outputbuf, mcontext);
	//return true if a random was expanded
	return expansion;
}

static void BotConstructChatMessage(bot_chatstate_t* chatstate, const char* message, unsigned mcontext,
	bot_matchvariable_t* variables, unsigned vcontext, bool reply)
{
	char srcmessage[MAX_MESSAGE_SIZE];
	String::Cpy(srcmessage, message);
	int i;
	for (i = 0; i < 10; i++)
	{
		if (!BotExpandChatMessage(chatstate->chatmessage, srcmessage, mcontext, variables, vcontext, reply))
		{
			break;
		}
		String::Cpy(srcmessage, chatstate->chatmessage);
	}
	if (i >= 10)
	{
		BotImport_Print(PRT_WARNING, "too many expansions in chat message\n");
		BotImport_Print(PRT_WARNING, "%s\n", chatstate->chatmessage);
	}
}

// randomly chooses one of the chat message of the given type
static const char* BotChooseInitialChatMessage(bot_chatstate_t* cs, const char* type)
{
	bot_chat_t* chat = cs->chat;
	for (bot_chattype_t* t = chat->types; t; t = t->next)
	{
		if (!String::ICmp(t->name, type))
		{
			int numchatmessages = 0;
			for (bot_chatmessage_t* m = t->firstchatmessage; m; m = m->next)
			{
				if (m->time > AAS_Time())
				{
					continue;
				}
				numchatmessages++;
			}
			//if all chat messages have been used recently
			if (numchatmessages <= 0)
			{
				float besttime = 0;
				bot_chatmessage_t* bestchatmessage = NULL;
				for (bot_chatmessage_t* m = t->firstchatmessage; m; m = m->next)
				{
					if (!besttime || m->time < besttime)
					{
						bestchatmessage = m;
						besttime = m->time;
					}
				}
				if (bestchatmessage)
				{
					return bestchatmessage->chatmessage;
				}
			}
			else//choose a chat message randomly
			{
				int n = random() * numchatmessages;
				for (bot_chatmessage_t* m = t->firstchatmessage; m; m = m->next)
				{
					if (m->time > AAS_Time())
					{
						continue;
					}
					if (--n < 0)
					{
						m->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
						return m->chatmessage;
					}
				}
			}
			return NULL;
		}
	}
	return NULL;
}

int BotNumInitialChats(int chatstate, const char* type)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}

	for (bot_chattype_t* t = cs->chat->types; t; t = t->next)
	{
		if (!String::ICmp(t->name, type))
		{
			if (LibVarGetValue("bot_testichat"))
			{
				BotImport_Print(PRT_MESSAGE, "%s has %d chat lines\n", type, t->numchatmessages);
				BotImport_Print(PRT_MESSAGE, "-------------------\n");
			}
			return t->numchatmessages;
		}
	}
	return 0;
}

void BotInitialChat(int chatstate, const char* type, int mcontext,
	const char* var0, const char* var1, const char* var2, const char* var3,
	const char* var4, const char* var5, const char* var6, const char* var7)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
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
	const char* message = BotChooseInitialChatMessage(cs, type);
	//if there's no message of the given type
	if (!message)
	{
#ifdef DEBUG
		BotImport_Print(PRT_MESSAGE, "no chat messages of type %s\n", type);
#endif
		return;
	}

	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
	Com_Memset(variables, 0, sizeof(variables));
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

	BotConstructChatMessage(cs, message, mcontext, variables, 0, false);
}

bool BotReplyChat(int chatstate, const char* message, int mcontext, int vcontext,
	const char* var0, const char* var1, const char* var2, const char* var3,
	const char* var4, const char* var5, const char* var6, const char* var7)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return false;
	}
	bot_match_t match;
	Com_Memset(&match, 0, sizeof(bot_match_t));
	String::Cpy(match.string, message);
	int bestpriority = -1;
	bot_chatmessage_t* bestchatmessage = NULL;
	bot_replychat_t* bestrchat = NULL;
	bot_match_t bestmatch;
	//go through all the reply chats
	for (bot_replychat_t* rchat = replychats; rchat; rchat = rchat->next)
	{
		bool found = false;
		for (bot_replychatkey_t* key = rchat->keys; key; key = key->next)
		{
			bool res = false;
			//get the match result
			if (key->flags & RCKFL_NAME)
			{
				res = (StringContains(message, cs->name, false) != -1);
			}
			else if (key->flags & RCKFL_BOTNAMES)
			{
				res = (StringContains(key->string, cs->name, false) != -1);
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
				res = (StringContainsWord(const_cast<char*>(message), key->string, false) != NULL);
			}
			//if the key must be present
			if (key->flags & RCKFL_AND)
			{
				if (!res)
				{
					found = false;
					break;
				}
			}
			//if the key must be absent
			else if (key->flags & RCKFL_NOT)
			{
				if (res)
				{
					found = false;
					break;
				}
			}
			else if (res)
			{
				found = true;
			}
		}

		if (found)
		{
			if (rchat->priority > bestpriority)
			{
				int numchatmessages = 0;
				bot_chatmessage_t* m;
				for (m = rchat->firstchatmessage; m; m = m->next)
				{
					if (m->time > AAS_Time())
					{
						continue;
					}
					numchatmessages++;
				}
				int num = random() * numchatmessages;
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
				}
				//if the reply chat has a message
				if (m)
				{
					Com_Memcpy(&bestmatch, &match, sizeof(bot_match_t));
					bestchatmessage = m;
					bestrchat = rchat;
					bestpriority = rchat->priority;
				}
			}
		}
	}
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
			for (bot_chatmessage_t* m = bestrchat->firstchatmessage; m; m = m->next)
			{
				BotConstructChatMessage(cs, m->chatmessage, mcontext, bestmatch.variables, vcontext, true);
				BotRemoveTildes(cs->chatmessage);
				BotImport_Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
			}
		}
		else
		{
			bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
			BotConstructChatMessage(cs, bestchatmessage->chatmessage, mcontext, bestmatch.variables, vcontext, true);
		}
		return true;
	}
	return false;
}

int BotChatLength(int chatstate)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return 0;
	}
	return String::Length(cs->chatmessage);
}

void BotGetChatMessage(int chatstate, char* buf, int size)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}

	BotRemoveTildes(cs->chatmessage);
	String::NCpy(buf, cs->chatmessage, size - 1);
	buf[size - 1] = '\0';
	//clear the chat message from the state
	String::Cpy(cs->chatmessage, "");
}

void BotSetChatGender(int chatstate, int gender)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	switch (gender)
	{
	case CHAT_GENDERFEMALE: cs->gender = CHAT_GENDERFEMALE; break;
	case CHAT_GENDERMALE: cs->gender = CHAT_GENDERMALE; break;
	default: cs->gender = CHAT_GENDERLESS; break;
	}
}

void BotSetChatName(int chatstate, const char* name, int client)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	cs->client = client;
	Com_Memset(cs->name, 0, sizeof(cs->name));
	String::NCpyZ(cs->name, name, sizeof(cs->name));
}

int BotAllocChatState()
{
	for (int i = 1; i <= MAX_BOTLIB_CLIENTS; i++)
	{
		if (!botchatstates[i])
		{
			botchatstates[i] = (bot_chatstate_t*)Mem_ClearedAlloc(sizeof(bot_chatstate_t));
			return i;
		}
	}
	return 0;
}

void BotFreeChatState(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "chat state handle %d out of range\n", handle);
		return;
	}
	if (!botchatstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid chat state %d\n", handle);
		return;
	}
	if (LibVarGetValue("bot_reloadcharacters"))
	{
		BotFreeChatFile(handle);
	}
	//free all the console messages left in the chat state
	if (GGameType & GAME_Quake3)
	{
		bot_consolemessage_q3_t m;
		for (int h = BotNextConsoleMessageQ3(handle, &m); h; h = BotNextConsoleMessageQ3(handle, &m))
		{
			//remove the console message
			BotRemoveConsoleMessage(handle, h);
		}
	}
	else
	{
		bot_consolemessage_wolf_t m;
		for (int h = BotNextConsoleMessageWolf(handle, &m); h; h = BotNextConsoleMessageWolf(handle, &m))
		{
			//remove the console message
			BotRemoveConsoleMessage(handle, h);
		}
	}
	Mem_Free(botchatstates[handle]);
	botchatstates[handle] = NULL;
}

int BotSetupChatAI()
{
#ifdef DEBUG
	int starttime = Sys_Milliseconds();
#endif

	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	const char* file = LibVarString("synfile", "syn.c");
	synonyms = BotLoadSynonyms(file);
	file = LibVarString("rndfile", "rnd.c");
	randomstrings = BotLoadRandomStrings(file);
	file = LibVarString("matchfile", "match.c");
	matchtemplates = BotLoadMatchTemplates(file);

	if (!LibVarValue("nochat", "0"))
	{
		file = LibVarString("rchatfile", "rchat.c");
		replychats = BotLoadReplyChat(file);
	}
	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder("");
	}

	InitConsoleMessageHeap();

#ifdef DEBUG
	BotImport_Print(PRT_MESSAGE, "setup chat AI %d msec\n", Sys_Milliseconds() - starttime);
#endif
	return BLERR_NOERROR;
}

void BotShutdownChatAI()
{
	//free all remaining chat states
	for (int i = 0; i < MAX_BOTLIB_CLIENTS; i++)
	{
		if (botchatstates[i])
		{
			BotFreeChatState(i);
		}
	}
	//free all cached chats
	for (int i = 0; i < MAX_BOTLIB_CLIENTS; i++)
	{
		if (ichatdata[i])
		{
			Mem_Free(ichatdata[i]->chat);
			Mem_Free(ichatdata[i]);
			ichatdata[i] = NULL;
		}
	}
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
		Mem_Free(randomstrings);
	}
	randomstrings = NULL;
	if (synonyms)
	{
		Mem_Free(synonyms);
	}
	synonyms = NULL;
	if (replychats)
	{
		BotFreeReplyChat(replychats);
	}
	replychats = NULL;
}

void BotEnterChatQ3(int chatstate, int clientto, int sendto)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
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
			switch (sendto)
			{
			case CHAT_TEAM:
				EA_Command(cs->client, va("say_team %s", cs->chatmessage));
				break;
			case CHAT_TELL:
				EA_Command(cs->client, va("tell %d %s", clientto, cs->chatmessage));
				break;
			default:	//CHAT_ALL
				EA_Command(cs->client, va("say %s", cs->chatmessage));
				break;
			}
		}
		//clear the chat message from the state
		String::Cpy(cs->chatmessage, "");
	}
}

void BotEnterChatWolf(int chatstate, int client, int sendto)
{
	bot_chatstate_t* cs = BotChatStateFromHandle(chatstate);
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
	}
}
