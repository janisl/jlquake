/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

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
#include "../game/botlib.h"
#include "be_interface.h"
#include "../game/be_ea.h"
#include "../game/be_ai_chat.h"


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
	bot_matchvariable_t variables[MAX_MATCHVARIABLES];
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
int BotReplyChat(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7)
{
	bot_replychat_t* rchat, * bestrchat;
	bot_replychatkey_t* key;
	bot_chatmessage_t* m, * bestchatmessage;
	bot_match_t match, bestmatch;
	int bestpriority, num, found, res, numchatmessages;
	bot_chatstate_t* cs;

	cs = BotChatStateFromHandle(chatstate);
	if (!cs)
	{
		return qfalse;
	}
	memset(&match, 0, sizeof(bot_match_t));
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
					memcpy(&bestmatch, &match, sizeof(bot_match_t));
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

	for (i = 1; i <= MAX_CLIENTS_WS; i++)
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
	bot_consolemessage_wolf_t m;
	int h;

	if (handle <= 0 || handle > MAX_CLIENTS_WS)
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
	for (i = 0; i < MAX_CLIENTS_WS; i++)
	{
		if (botchatstates[i])
		{
			BotFreeChatState(i);
		}	//end if
	}	//end for
		//free all cached chats
	for (i = 0; i < MAX_CLIENTS_WS; i++)
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
