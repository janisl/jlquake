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
		cm->next = NULL;
		cm->prev = NULL;
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

void StringReplaceWords(char* string, const char* synonym, const char* replacement)
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
