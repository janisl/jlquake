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


#include "server.h"

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
void SV_SendMessageToClient(QMsg* msg, client_t* client)
{
	int rateMsec;

	// record information about the message
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageSize = msg->cursize;
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageSent = svs.q3_time;
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageAcked = -1;

	// send the datagram
	SV_Netchan_Transmit(client, msg);	//msg->cursize, msg->data );

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	if (client->netchan.remoteAddress.type == NA_LOOPBACK || SOCK_IsLANAddress(client->netchan.remoteAddress))
	{
		client->q3_nextSnapshotTime = svs.q3_time - 1;
		return;
	}

	// normal rate / snapshotMsec calculation
	rateMsec = SVT3_RateMsec(client, msg->cursize);

	if (rateMsec < client->q3_snapshotMsec)
	{
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->q3_snapshotMsec;
		client->q3_rateDelayed = qfalse;
	}
	else
	{
		client->q3_rateDelayed = qtrue;
	}

	client->q3_nextSnapshotTime = svs.q3_time + rateMsec;

	// don't pile up empty snapshots while connecting
	if (client->state != CS_ACTIVE)
	{
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if (!*client->downloadName && client->q3_nextSnapshotTime < svs.q3_time + 1000)
		{
			client->q3_nextSnapshotTime = svs.q3_time + 1000;
		}
	}
}


/*
=======================
SV_SendClientSnapshot

Also called by SV_FinalMessage

=======================
*/
void SV_SendClientSnapshot(client_t* client)
{
	byte msg_buf[MAX_MSGLEN_WOLF];
	QMsg msg;

	//RF, AI don't need snapshots built
	if (client->ws_gentity && client->ws_gentity->r.svFlags & WSSVF_CASTAI)
	{
		return;
	}

	// build the snapshot
	SVT3_BuildClientSnapshot(client);

	// bots need to have their snapshots build, but
	// the query them directly without needing to be sent
	if (client->ws_gentity && client->ws_gentity->r.svFlags & Q3SVF_BOT)
	{
		return;
	}

	msg.Init(msg_buf, sizeof(msg_buf));
	msg.allowoverflow = qtrue;

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// (re)send any reliable server commands
	SVT3_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant wsentityState_t
	// and the wsplayerState_t
	SVT3_WriteSnapshotToClient(client, &msg);

	// Add any download data if the client is downloading
	SV_WriteDownloadToClient(client, &msg);

	// check for overflow
	if (msg.overflowed)
	{
		Com_Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	SV_SendMessageToClient(&msg, client);
}


/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages(void)
{
	int i;
	client_t* c;

	// send a message to each connected client
	for (i = 0, c = svs.clients; i < sv_maxclients->integer; i++, c++)
	{
		if (!c->state)
		{
			continue;		// not connected
		}

		if (svs.q3_time < c->q3_nextSnapshotTime)
		{
			continue;		// not time yet
		}

		// send additional message fragments if the last message
		// was too large to send at once
		if (c->netchan.unsentFragments)
		{
			c->q3_nextSnapshotTime = svs.q3_time +
								  SVT3_RateMsec(c, c->netchan.reliableOrUnsentLength - c->netchan.unsentFragmentStart);
			SV_Netchan_TransmitNextFragment(&c->netchan);
			continue;
		}

		// generate and send a new message
		SV_SendClientSnapshot(c);
	}
}
