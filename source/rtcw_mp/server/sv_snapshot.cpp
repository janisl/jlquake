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
	SV_Netchan_Transmit(client, msg);

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	// TTimo - show_bug.cgi?id=491
	// added svt3_lanForceRate check
	if (client->netchan.remoteAddress.type == NA_LOOPBACK || (svt3_lanForceRate->integer && SOCK_IsLANAddress(client->netchan.remoteAddress)))
	{
		client->q3_nextSnapshotTime = svs.q3_time - 1;
		return;
	}

	// normal rate / snapshotMsec calculation
	rateMsec = SVT3_RateMsec(client, msg->cursize);

	// TTimo - during a download, ignore the snapshotMsec
	// the update server on steroids, with this disabled and sv_fps 60, the download can reach 30 kb/s
	// on a regular server, we will still top at 20 kb/s because of sv_fps 20
	if (!*client->downloadName && rateMsec < client->q3_snapshotMsec)
	{
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->q3_snapshotMsec;
		client->q3_rateDelayed = false;
	}
	else
	{
		client->q3_rateDelayed = true;
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

	// build the snapshot
	SVT3_BuildClientSnapshot(client);

	// bots need to have their snapshots build, but
	// the query them directly without needing to be sent
	if (client->wm_gentity && client->wm_gentity->r.svFlags & Q3SVF_BOT)
	{
		return;
	}

	msg.Init(msg_buf, sizeof(msg_buf));
	msg.allowoverflow = true;

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// (re)send any reliable server commands
	SVT3_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant wmentityState_t
	// and the wmplayerState_t
	SVT3_WriteSnapshotToClient(client, &msg);

	// Add any download data if the client is downloading
	SVT3_WriteDownloadToClient(client, &msg);

	// check for overflow
	if (msg.overflowed)
	{
		common->Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	SV_SendMessageToClient(&msg, client);

	sv.wm_bpsTotalBytes += msg.cursize;			// NERVE - SMF - net debugging
	sv.wm_ubpsTotalBytes += msg.uncompsize / 8;	// NERVE - SMF - net debugging
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
	int numclients = 0;			// NERVE - SMF - net debugging

	sv.wm_bpsTotalBytes = 0;		// NERVE - SMF - net debugging
	sv.wm_ubpsTotalBytes = 0;		// NERVE - SMF - net debugging

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

		numclients++;		// NERVE - SMF - net debugging

		// send additional message fragments if the last message
		// was too large to send at once
		if (c->netchan.unsentFragments)
		{
			c->q3_nextSnapshotTime = svs.q3_time +
								  SVT3_RateMsec(c, c->netchan.reliableOrUnsentLength - c->netchan.unsentFragmentStart);
			SV_Netchan_TransmitNextFragment(c);
			continue;
		}

		// generate and send a new message
		SV_SendClientSnapshot(c);
	}

	// NERVE - SMF - net debugging
	if (sv_showAverageBPS->integer && numclients > 0)
	{
		float ave = 0, uave = 0;

		for (i = 0; i < MAX_BPS_WINDOW - 1; i++)
		{
			sv.wm_bpsWindow[i] = sv.wm_bpsWindow[i + 1];
			ave += sv.wm_bpsWindow[i];

			sv.wm_ubpsWindow[i] = sv.wm_ubpsWindow[i + 1];
			uave += sv.wm_ubpsWindow[i];
		}

		sv.wm_bpsWindow[MAX_BPS_WINDOW - 1] = sv.wm_bpsTotalBytes;
		ave += sv.wm_bpsTotalBytes;

		sv.wm_ubpsWindow[MAX_BPS_WINDOW - 1] = sv.wm_ubpsTotalBytes;
		uave += sv.wm_ubpsTotalBytes;

		if (sv.wm_bpsTotalBytes >= sv.wm_bpsMaxBytes)
		{
			sv.wm_bpsMaxBytes = sv.wm_bpsTotalBytes;
		}

		if (sv.wm_ubpsTotalBytes >= sv.wm_ubpsMaxBytes)
		{
			sv.wm_ubpsMaxBytes = sv.wm_ubpsTotalBytes;
		}

		sv.wm_bpsWindowSteps++;

		if (sv.wm_bpsWindowSteps >= MAX_BPS_WINDOW)
		{
			float comp_ratio;

			sv.wm_bpsWindowSteps = 0;

			ave = (ave / (float)MAX_BPS_WINDOW);
			uave = (uave / (float)MAX_BPS_WINDOW);

			comp_ratio = (1 - ave / uave) * 100.f;
			sv.wm_ucompAve += comp_ratio;
			sv.wm_ucompNum++;

			common->DPrintf("bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
				ave / (float)numclients, ave, sv.wm_bpsMaxBytes, uave, sv.wm_ubpsMaxBytes, comp_ratio, sv.wm_ucompAve / sv.wm_ucompNum);
		}
	}
	// -NERVE - SMF
}
