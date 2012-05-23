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
 * name:		be_ai_chat.c
 *
 * desc:		bot chat AI
 *
 * $Archive: /MissionPack/code/botlib/be_ai_chat.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_ea.h"
#include "be_ai_chat.h"

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
	int index;
	bot_match_t match;
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
	Com_Memset(&match, 0, sizeof(match));
	index = 0;
	if (var0)
	{
		String::Cat(match.string, sizeof(match.string), var0);
		match.variables[0].ptr = match.string + index;
		match.variables[0].length = String::Length(var0);
		index += String::Length(var0);
	}
	if (var1)
	{
		String::Cat(match.string, sizeof(match.string), var1);
		match.variables[1].ptr = match.string + index;
		match.variables[1].length = String::Length(var1);
		index += String::Length(var1);
	}
	if (var2)
	{
		String::Cat(match.string, sizeof(match.string), var2);
		match.variables[2].ptr = match.string + index;
		match.variables[2].length = String::Length(var2);
		index += String::Length(var2);
	}
	if (var3)
	{
		String::Cat(match.string, sizeof(match.string), var3);
		match.variables[3].ptr = match.string + index;
		match.variables[3].length = String::Length(var3);
		index += String::Length(var3);
	}
	if (var4)
	{
		String::Cat(match.string, sizeof(match.string), var4);
		match.variables[4].ptr = match.string + index;
		match.variables[4].length = String::Length(var4);
		index += String::Length(var4);
	}
	if (var5)
	{
		String::Cat(match.string, sizeof(match.string), var5);
		match.variables[5].ptr = match.string + index;
		match.variables[5].length = String::Length(var5);
		index += String::Length(var5);
	}
	if (var6)
	{
		String::Cat(match.string, sizeof(match.string), var6);
		match.variables[6].ptr = match.string + index;
		match.variables[6].length = String::Length(var6);
		index += String::Length(var6);
	}
	if (var7)
	{
		String::Cat(match.string, sizeof(match.string), var7);
		match.variables[7].ptr = match.string + index;
		match.variables[7].length = String::Length(var7);
		index += String::Length(var7);
	}
	//
	BotConstructChatMessage(cs, message, mcontext, match.variables, 0, false);
}	//end of the function BotInitialChat
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
	bot_match_t match, bestmatch;
	int bestpriority, num, found, res, numchatmessages, index;
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return false;
	}
	Com_Memset(&match, 0, sizeof(bot_match_t));
	String::Cpy(match.string, message);
	bestpriority = -1;
	bestchatmessage = NULL;
	bestrchat = NULL;
	//go through all the reply chats
	for (rchat = replychats; rchat; rchat = rchat->next)
	{
		found = false;
		for (key = rchat->keys; key; key = key->next)
		{
			res = false;
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
				res = (StringContainsWord(message, key->string, false) != NULL);
			}
			//if the key must be present
			if (key->flags & RCKFL_AND)
			{
				if (!res)
				{
					found = false;
					break;
				}	//end if
			}	//end else if
				//if the key must be absent
			else if (key->flags & RCKFL_NOT)
			{
				if (res)
				{
					found = false;
					break;
				}	//end if
			}	//end if
			else if (res)
			{
				found = true;
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
					Com_Memcpy(&bestmatch, &match, sizeof(bot_match_t));
					bestchatmessage = m;
					bestrchat = rchat;
					bestpriority = rchat->priority;
				}	//end if
			}	//end if
		}	//end if
	}	//end for
	if (bestchatmessage)
	{
		index = String::Length(bestmatch.string);
		if (var0)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var0);
			bestmatch.variables[0].ptr = bestmatch.string + index;
			bestmatch.variables[0].length = String::Length(var0);
			index += String::Length(var0);
		}
		if (var1)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var1);
			bestmatch.variables[1].ptr = bestmatch.string + index;
			bestmatch.variables[1].length = String::Length(var1);
			index += String::Length(var1);
		}
		if (var2)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var2);
			bestmatch.variables[2].ptr = bestmatch.string + index;
			bestmatch.variables[2].length = String::Length(var2);
			index += String::Length(var2);
		}
		if (var3)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var3);
			bestmatch.variables[3].ptr = bestmatch.string + index;
			bestmatch.variables[3].length = String::Length(var3);
			index += String::Length(var3);
		}
		if (var4)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var4);
			bestmatch.variables[4].ptr = bestmatch.string + index;
			bestmatch.variables[4].length = String::Length(var4);
			index += String::Length(var4);
		}
		if (var5)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var5);
			bestmatch.variables[5].ptr = bestmatch.string + index;
			bestmatch.variables[5].length = String::Length(var5);
			index += String::Length(var5);
		}
		if (var6)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var6);
			bestmatch.variables[6].ptr = bestmatch.string + index;
			bestmatch.variables[6].length = String::Length(var6);
			index += String::Length(var6);
		}
		if (var7)
		{
			String::Cat(bestmatch.string, sizeof(bestmatch.string), var7);
			bestmatch.variables[7].ptr = bestmatch.string + index;
			bestmatch.variables[7].length = String::Length(var7);
			index += String::Length(var7);
		}
		if (LibVarGetValue("bot_testrchat"))
		{
			for (m = bestrchat->firstchatmessage; m; m = m->next)
			{
				BotConstructChatMessage(cs, m->chatmessage, mcontext, bestmatch.variables, vcontext, true);
				BotRemoveTildes(cs->chatmessage);
				BotImport_Print(PRT_MESSAGE, "%s\n", cs->chatmessage);
			}	//end if
		}	//end if
		else
		{
			bestchatmessage->time = AAS_Time() + CHATMESSAGE_RECENTTIME;
			BotConstructChatMessage(cs, bestchatmessage->chatmessage, mcontext, bestmatch.variables, vcontext, true);
		}	//end else
		return true;
	}	//end if
	return false;
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
void BotEnterChat(int chatstate, int clientto, int sendto)
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
void BotSetChatName(int chatstate, char* name, int client)
{
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return;
	}
	cs->client = client;
	Com_Memset(cs->name, 0, sizeof(cs->name));
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

	for (i = 1; i <= MAX_CLIENTS_Q3; i++)
	{
		if (!botchatstates[i])
		{
			botchatstates[i] = (bot_chatstate_t*)Mem_ClearedAlloc(sizeof(bot_chatstate_t));
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
	bot_consolemessage_q3_t m;
	int h;

	if (handle <= 0 || handle > MAX_CLIENTS_Q3)
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
	for (h = BotNextConsoleMessageQ3(handle, &m); h; h = BotNextConsoleMessageQ3(handle, &m))
	{
		//remove the console message
		BotRemoveConsoleMessage(handle, h);
	}	//end for
	Mem_Free(botchatstates[handle]);
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
	for (i = 0; i < MAX_CLIENTS_Q3; i++)
	{
		if (botchatstates[i])
		{
			BotFreeChatState(i);
		}	//end if
	}	//end for
		//free all cached chats
	for (i = 0; i < MAX_CLIENTS_Q3; i++)
	{
		if (ichatdata[i])
		{
			Mem_Free(ichatdata[i]->chat);
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
}	//end of the function BotShutdownChatAI
