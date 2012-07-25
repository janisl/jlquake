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

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

/*
=================
SV_Netchan_TransmitNextFragment
=================
*/
void SV_Netchan_TransmitNextFragment(client_t* client)
{
	Netchan_TransmitNextFragment(&client->netchan);
	if (!client->netchan.unsentFragments)
	{
		// make sure the netchan queue has been properly initialized (you never know)
		if (!client->q3_netchan_end_queue)
		{
			Com_Error(ERR_DROP, "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment\n");
		}
		// the last fragment was transmitted, check wether we have queued messages
		if (client->q3_netchan_start_queue)
		{
			netchan_buffer_t* netbuf;
			Com_DPrintf("#462 Netchan_TransmitNextFragment: popping a queued message for transmit\n");
			netbuf = client->q3_netchan_start_queue;
			SVT3_Netchan_Encode(client, &netbuf->msg, client->q3_lastClientCommandString);
			Netchan_Transmit(&client->netchan, netbuf->msg.cursize, netbuf->msg._data);
			// pop from queue
			client->q3_netchan_start_queue = netbuf->next;
			if (!client->q3_netchan_start_queue)
			{
				Com_DPrintf("#462 Netchan_TransmitNextFragment: emptied queue\n");
				client->q3_netchan_end_queue = &client->q3_netchan_start_queue;
			}
			else
			{
				Com_DPrintf("#462 Netchan_TransmitNextFragment: remaining queued message\n");
			}
			Z_Free(netbuf);
		}
	}
}


/*
===============
SV_Netchan_Transmit
TTimo
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=462
if there are some unsent fragments (which may happen if the snapshots
and the gamestate are fragmenting, and collide on send for instance)
then buffer them and make sure they get sent in correct order
================
*/

void SV_Netchan_Transmit(client_t* client, QMsg* msg)		//int length, const byte *data ) {
{
	msg->WriteByte(q3svc_EOF);
	if (client->netchan.unsentFragments)
	{
		netchan_buffer_t* netbuf;
		Com_DPrintf("#462 SV_Netchan_Transmit: unsent fragments, stacked\n");
		netbuf = (netchan_buffer_t*)Z_Malloc(sizeof(netchan_buffer_t));
		// store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending
		netbuf->msg.Copy(netbuf->msgBuffer, sizeof(netbuf->msgBuffer), *msg);
		netbuf->next = NULL;
		// insert it in the queue, the message will be encoded and sent later
		*client->q3_netchan_end_queue = netbuf;
		client->q3_netchan_end_queue = &(*client->q3_netchan_end_queue)->next;
		// emit the next fragment of the current message for now
		Netchan_TransmitNextFragment(&client->netchan);
	}
	else
	{
		SVT3_Netchan_Encode(client, msg, client->q3_lastClientCommandString);
		Netchan_Transmit(&client->netchan, msg->cursize, msg->_data);
	}
}

/*
=================
Netchan_SV_Process
=================
*/
qboolean SV_Netchan_Process(client_t* client, QMsg* msg)
{
	int ret;
	ret = Netchan_Process(&client->netchan, msg);
	if (!ret)
	{
		return false;
	}
	SVT3_Netchan_Decode(client, msg);
	return true;
}
