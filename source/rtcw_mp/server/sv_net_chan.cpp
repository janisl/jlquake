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


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

/*
==============
SV_Netchan_Encode

    // first four bytes of the data are always:
    long reliableAcknowledge;

==============
*/
static void SV_Netchan_Encode(client_t* client, QMsg* msg)
{
	long reliableAcknowledge, i, index;
	byte key, * string;
	int srdc, sbit, soob;

	if (msg->cursize < SV_ENCODE_START)
	{
		return;
	}

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->bit = 0;
	msg->readcount = 0;
	msg->oob = 0;

	reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte*)client->q3_lastClientCommandString;
	index = 0;
	// xor the client challenge with the netchan sequence number
	key = client->challenge ^ client->netchan.outgoingSequence;
	for (i = SV_ENCODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last received and with this message acknowledged client command
		if (!string[index])
		{
			index = 0;
		}
		if (string[index] > 127 || string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else
		{
			key ^= string[index] << (i & 1);
		}
		index++;
		// encode the data with this key
		*(msg->_data + i) = *(msg->_data + i) ^ key;
	}
}

/*
==============
SV_Netchan_Decode

    // first 12 bytes of the data are always:
    long serverId;
    long messageAcknowledge;
    long reliableAcknowledge;

==============
*/
static void SV_Netchan_Decode(client_t* client, QMsg* msg)
{
	int serverId, messageAcknowledge, reliableAcknowledge;
	int i, index, srdc, sbit, soob;
	byte key, * string;

	srdc = msg->readcount;
	sbit = msg->bit;
	soob = msg->oob;

	msg->oob = 0;

	serverId = msg->ReadLong();
	messageAcknowledge = msg->ReadLong();
	reliableAcknowledge = msg->ReadLong();

	msg->oob = soob;
	msg->bit = sbit;
	msg->readcount = srdc;

	string = (byte*)client->q3_reliableCommands[reliableAcknowledge & (MAX_RELIABLE_COMMANDS_WOLF - 1)];
	index = 0;
	//
	key = client->challenge ^ serverId ^ messageAcknowledge;
	for (i = msg->readcount + SV_DECODE_START; i < msg->cursize; i++)
	{
		// modify the key with the last sent and acknowledged server command
		if (!string[index])
		{
			index = 0;
		}
		if (string[index] > 127 || string[index] == '%')
		{
			key ^= '.' << (i & 1);
		}
		else
		{
			key ^= string[index] << (i & 1);
		}
		index++;
		// decode the data with this key
		*(msg->_data + i) = *(msg->_data + i) ^ key;
	}
}

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
			//Com_DPrintf("Netchan_TransmitNextFragment: popping a queued message for transmit\n");
			netbuf = client->q3_netchan_start_queue;

			SV_Netchan_Encode(client, &netbuf->msg);
			Netchan_Transmit(&client->netchan, netbuf->msg.cursize, netbuf->msg._data);

			// pop from queue
			client->q3_netchan_start_queue = netbuf->next;
			if (!client->q3_netchan_start_queue)
			{
				//Com_DPrintf("Netchan_TransmitNextFragment: emptied queue\n");
				client->q3_netchan_end_queue = &client->q3_netchan_start_queue;
			}
			/*
			else
			    Com_DPrintf("Netchan_TransmitNextFragment: remaining queued message\n");
			    */
			Z_Free(netbuf);
		}
	}
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
	if (client->netchan.unsentFragments)
	{
		netchan_buffer_t* netbuf;
		//Com_DPrintf("SV_Netchan_Transmit: there are unsent fragments remaining\n");
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
		SV_Netchan_Encode(client, msg);
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
	SV_Netchan_Decode(client, msg);
	return qtrue;
}
