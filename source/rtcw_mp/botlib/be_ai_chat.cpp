/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

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
				bot_match_t imatch;
				MatchWolfToInt(&match, &imatch);
				res = StringsMatch(key->match, &imatch);
				MatchIntToWolf(&imatch, &match);
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

	for (i = 1; i <= MAX_CLIENTS_WM; i++)
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

	if (handle <= 0 || handle > MAX_CLIENTS_WM)
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
	for (i = 0; i < MAX_CLIENTS_WM; i++)
	{
		if (botchatstates[i])
		{
			BotFreeChatState(i);
		}	//end if
	}	//end for
		//free all cached chats
	for (i = 0; i < MAX_CLIENTS_WM; i++)
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
