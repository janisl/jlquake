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
//!!!!!!!!!!!!!!! Used by game VM !!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
/*****************************************************************************
 * name:		be_ai_chat.h
 *
 * desc:		char AI
 *
 * $Archive: /source/code/botlib/be_ai_chat.h $
 *
 *****************************************************************************/

//setup the chat AI
int BotSetupChatAI(void);
//shutdown the chat AI
void BotShutdownChatAI(void);
//returns the handle to a newly allocated chat state
int BotAllocChatState(void);
//frees the chatstate
void BotFreeChatState(int handle);
//returns the length of the currently selected chat message
int BotChatLength(int chatstate);
//enters the selected chat message
void BotEnterChat(int chatstate, int clientto, int sendto);
//get the chat message ready to be output
void BotGetChatMessage(int chatstate, char* buf, int size);
//store the gender of the bot in the chat state
void BotSetChatGender(int chatstate, int gender);
//store the bot name in the chat state
void BotSetChatName(int chatstate, char* name, int client);
