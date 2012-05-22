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
 * name:		be_ai_chat.h
 *
 * desc:		char AI
 *
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
//enters a chat message of the given type
void BotInitialChat(int chatstate, char* type, int mcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
//returns the number of initial chat messages of the given type
int BotNumInitialChats(int chatstate, char* type);
//find a reply for the given message
int BotReplyChat(int chatstate, char* message, int mcontext, int vcontext, char* var0, char* var1, char* var2, char* var3, char* var4, char* var5, char* var6, char* var7);
//returns the length of the currently selected chat message
int BotChatLength(int chatstate);
//enters the selected chat message
void BotEnterChat(int chatstate, int client, int sendto);
//get the chat message ready to be output
void BotGetChatMessage(int chatstate, char* buf, int size);
//loads a chat file for the chat state
int BotLoadChatFile(int chatstate, char* chatfile, char* chatname);
//store the gender of the bot in the chat state
void BotSetChatGender(int chatstate, int gender);
//store the bot name in the chat state
void BotSetChatName(int chatstate, char* name);
