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
