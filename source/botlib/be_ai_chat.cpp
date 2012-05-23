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
