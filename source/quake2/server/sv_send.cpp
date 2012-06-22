/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_main.c -- server main program

#include "server.h"

/*
=============================================================================

Com_Printf redirection

=============================================================================
*/

char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect(int sv_redirected, char* outputbuf)
{
	if (sv_redirected == RD_PACKET)
	{
		Netchan_OutOfBandPrint(NS_SERVER, net_from, "print\n%s", outputbuf);
	}
	else if (sv_redirected == RD_CLIENT)
	{
		sv_client->netchan.message.WriteByte(q2svc_print);
		sv_client->netchan.message.WriteByte(PRINT_HIGH);
		sv_client->netchan.message.WriteString2(outputbuf);
	}
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/



/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram(client_t* client)
{
	byte msg_buf[MAX_MSGLEN_Q2];
	QMsg msg;

	SVQ2_BuildClientFrame(client);

	msg.InitOOB(msg_buf, sizeof(msg_buf));
	msg.allowoverflow = true;

	// send over all the relevant q2entity_state_t
	// and the q2player_state_t
	SVQ2_WriteFrameToClient(client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	// it is necessary for this to be after the WriteEntities
	// so that entity references will be current
	if (client->datagram.overflowed)
	{
		Com_Printf("WARNING: datagram overflowed for %s\n", client->name);
	}
	else
	{
		msg.WriteData(client->datagram._data, client->datagram.cursize);
	}
	client->datagram.Clear();

	if (msg.overflowed)
	{	// must have room left for the packet header
		Com_Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();
	}

	// send the datagram
	Netchan_Transmit(&client->netchan, msg.cursize, msg._data);

	// record the size for rate estimation
	client->q2_message_size[sv.q2_framenum % RATE_MESSAGES] = msg.cursize;

	return true;
}


/*
==================
SV_DemoCompleted
==================
*/
void SV_DemoCompleted(void)
{
	if (sv.q2_demofile)
	{
		FS_FCloseFile(sv.q2_demofile);
		sv.q2_demofile = 0;
	}
	SV_Nextserver();
}


/*
=======================
SV_RateDrop

Returns true if the client is over its current
bandwidth estimation and should not be sent another packet
=======================
*/
qboolean SV_RateDrop(client_t* c)
{
	int total;
	int i;

	// never drop over the loopback
	if (c->netchan.remoteAddress.type == NA_LOOPBACK)
	{
		return false;
	}

	total = 0;

	for (i = 0; i < RATE_MESSAGES; i++)
	{
		total += c->q2_message_size[i];
	}

	if (total > c->rate)
	{
		c->q2_surpressCount++;
		c->q2_message_size[sv.q2_framenum % RATE_MESSAGES] = 0;
		return true;
	}

	return false;
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
	int msglen;
	byte msgbuf[MAX_MSGLEN_Q2];
	int r;

	msglen = 0;

	// read the next demo message if needed
	if (sv.state == SS_DEMO && sv.q2_demofile)
	{
		if (sv_paused->value)
		{
			msglen = 0;
		}
		else
		{
			// get the next message
			r = FS_Read(&msglen, 4, sv.q2_demofile);
			if (r != 4)
			{
				SV_DemoCompleted();
				return;
			}
			msglen = LittleLong(msglen);
			if (msglen == -1)
			{
				SV_DemoCompleted();
				return;
			}
			if (msglen > MAX_MSGLEN_Q2)
			{
				Com_Error(ERR_DROP, "SV_SendClientMessages: msglen > MAX_MSGLEN_Q2");
			}
			r = FS_Read(msgbuf, msglen, sv.q2_demofile);
			if (r != msglen)
			{
				SV_DemoCompleted();
				return;
			}
		}
	}

	// send a message to each connected client
	for (i = 0, c = svs.clients; i < sv_maxclients->value; i++, c++)
	{
		if (!c->state)
		{
			continue;
		}
		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			c->netchan.message.Clear();
			c->datagram.Clear();
			SVQ2_BroadcastPrintf(PRINT_HIGH, "%s overflowed\n", c->name);
			SV_DropClient(c);
		}

		if (sv.state == SS_CINEMATIC ||
			sv.state == SS_DEMO ||
			sv.state == SS_PIC
			)
		{
			Netchan_Transmit(&c->netchan, msglen, msgbuf);
		}
		else if (c->state == CS_ACTIVE)
		{
			// don't overrun bandwidth
			if (SV_RateDrop(c))
			{
				continue;
			}

			SV_SendClientDatagram(c);
		}
		else
		{
			// just update reliable	if needed
			if (c->netchan.message.cursize  || curtime - c->netchan.lastSent > 1000)
			{
				Netchan_Transmit(&c->netchan, 0, NULL);
			}
		}
	}
}
