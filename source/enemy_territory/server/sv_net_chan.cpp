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
	while (!client->netchan.unsentFragments && client->q3_netchan_start_queue)
	{
		// make sure the netchan queue has been properly initialized (you never know)
		//%	if (!client->netchan_end_queue) {
		//%		Com_Error(ERR_DROP, "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment\n");
		//%	}
		// the last fragment was transmitted, check wether we have queued messages
		netchan_buffer_t* netbuf = client->q3_netchan_start_queue;

		// pop from queue
		client->q3_netchan_start_queue = netbuf->next;
		if (!client->q3_netchan_start_queue)
		{
			client->et_netchan_end_queue = NULL;
		}

		if (!SVET_GameIsSinglePlayer())
		{
			SVT3_Netchan_Encode(client, &netbuf->msg, netbuf->lastClientCommandString);
		}
		Netchan_Transmit(&client->netchan, netbuf->msg.cursize, netbuf->msg._data);

		Z_Free(netbuf);
	}
}

/*
===============
SV_WriteBinaryMessage
===============
*/
static void SV_WriteBinaryMessage(QMsg* msg, client_t* cl)
{
	if (!cl->et_binaryMessageLength)
	{
		return;
	}

	msg->Uncompressed();

	if ((msg->cursize + cl->et_binaryMessageLength) >= msg->maxsize)
	{
		cl->et_binaryMessageOverflowed = qtrue;
		return;
	}

	msg->WriteData(cl->et_binaryMessage, cl->et_binaryMessageLength);
	cl->et_binaryMessageLength = 0;
	cl->et_binaryMessageOverflowed = qfalse;
}

/*
===============
SV_Netchan_Transmit

TTimo
show_bug.cgi?id=462
if there are some unsent fragments (which may happen if the snapshots
and the gamestate are fragmenting, and collide on send for instance)
then buffer them and make sure they get sent in correct order
================
*/
void SV_Netchan_Transmit(client_t* client, QMsg* msg)		//int length, const byte *data ) {
{
	msg->WriteByte(q3svc_EOF);
	SV_WriteBinaryMessage(msg, client);

	if (client->netchan.unsentFragments)
	{
		netchan_buffer_t* netbuf;
		//Com_DPrintf("SV_Netchan_Transmit: there are unsent fragments remaining\n");
		netbuf = (netchan_buffer_t*)Z_Malloc(sizeof(netchan_buffer_t));

		// store the msg, we can't store it encoded, as the encoding depends on stuff we still have to finish sending
		netbuf->msg.Copy(netbuf->msgBuffer, sizeof(netbuf->msgBuffer), *msg);

		// copy the command, since the command number used for encryption is
		// already compressed in the buffer, and receiving a new command would
		// otherwise lose the proper encryption key
		String::Cpy(netbuf->lastClientCommandString, client->q3_lastClientCommandString);

		// insert it in the queue, the message will be encoded and sent later
		//%	*client->netchan_end_queue = netbuf;
		//%	client->netchan_end_queue = &(*client->netchan_end_queue)->next;
		netbuf->next = NULL;
		if (!client->q3_netchan_start_queue)
		{
			client->q3_netchan_start_queue = netbuf;
		}
		else
		{
			client->et_netchan_end_queue->next = netbuf;
		}
		client->et_netchan_end_queue = netbuf;

		// emit the next fragment of the current message for now
		Netchan_TransmitNextFragment(&client->netchan);
	}
	else
	{
		if (!SVET_GameIsSinglePlayer())
		{
			SVT3_Netchan_Encode(client, msg, client->q3_lastClientCommandString);
		}
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
		return qfalse;
	}
	if (!SVET_GameIsSinglePlayer())
	{
		SVT3_Netchan_Decode(client, msg);
	}
	return qtrue;
}
