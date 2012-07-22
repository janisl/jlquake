//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "qcommon.h"

struct delaybuf_t
{
	netsrc_t sock;
	int length;
	char data[MAX_PACKETLEN];
	netadr_t to;
	int time;
	delaybuf_t* next;
};

Cvar* sv_hostname;

loopback_t loopbacks[2];

static unsigned char huffbuff[65536];

int ip_sockets[2];

Cvar* sv_packetloss;
Cvar* sv_packetdelay;
Cvar* cl_packetloss;
Cvar* cl_packetdelay;
Cvar* showpackets;
Cvar* qport;
Cvar* showdrop;

static delaybuf_t* sv_delaybuf_head = NULL;
static delaybuf_t* sv_delaybuf_tail = NULL;
static delaybuf_t* cl_delaybuf_head = NULL;
static delaybuf_t* cl_delaybuf_tail = NULL;

const char* netsrcString[2] =
{
	"client",
	"server"
};

int NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	loopback_t* loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
	{
		loop->get = loop->send - MAX_LOOPBACK;
	}

	if (loop->get >= loop->send)
	{
		return false;
	}

	int i = loop->get & (MAX_LOOPBACK - 1);
	loop->get++;

	Com_Memcpy(net_message->_data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	Com_Memset(net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	return loop->msgs[i].type;
}

void NET_SendLoopPacket(netsrc_t sock, int length, const void* data, int type)
{
	loopback_t* loop = &loopbacks[sock ^ 1];

	int i = loop->send & (MAX_LOOPBACK - 1);
	loop->send++;

	Com_Memcpy(loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
	loop->msgs[i].type = type;
}

bool NET_GetUdpPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	int net_socket = ip_sockets[sock];

	if (!net_socket)
	{
		return false;
	}

	int ret = SOCK_Recv(net_socket, net_message->_data, net_message->maxsize, net_from);
	if (ret == SOCKRECV_NO_DATA)
	{
		return false;
	}
	if (ret == SOCKRECV_ERROR)
	{
		if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld) ||
			(GGameType & GAME_Quake2 && !com_dedicated->value))	// let dedicated servers continue after errors
		{
			common->Error("NET_GetPacket failed");
		}
		return false;
	}

	if (ret == net_message->maxsize)
	{
		common->Printf("Oversize packet from %s\n", SOCK_AdrToString(*net_from));
		return false;
	}

	if (GGameType & GAME_HexenWorld)
	{
		Com_Memcpy(huffbuff, net_message->_data, ret);
		HuffDecode(huffbuff, net_message->_data, ret, &ret);
	}

	net_message->cursize = ret;
	net_message->readcount = 0;
	return true;
}

bool NET_GetPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message)
{
	if (NET_GetLoopPacket(sock, net_from, net_message))
	{
		return true;
	}

	return NET_GetUdpPacket(sock, net_from, net_message);
}

static void NET_DoSendPacket(netsrc_t sock, int length, const void* data, const netadr_t& to)
{
	// sequenced packets are shown in netchan, so just show oob
	if (showpackets->integer && *(int*)data == -1)
	{
		common->Printf("send packet %4i\n", length);
	}

	if (to.type == NA_LOOPBACK)
	{
		NET_SendLoopPacket(sock, length, data, 1);
		return;
	}
	if (to.type == NA_BOT)
	{
		return;
	}
	if (to.type == NA_BAD)
	{
		return;
	}

	if (to.type != NA_BROADCAST && to.type != NA_IP)
	{
		common->FatalError("NET_SendPacket: bad address type");
		return;
	}

	if (!ip_sockets[sock])
	{
		return;
	}

	if (GGameType & GAME_HexenWorld)
	{
		int outlen;
		HuffEncode((unsigned char*)data, huffbuff, length, &outlen);
		SOCK_Send(ip_sockets[sock], huffbuff, outlen, to);
	}
	else
	{
		int ret = SOCK_Send(ip_sockets[sock], data, length, to);
		if (GGameType & GAME_Quake2 && ret == SOCKSEND_ERROR)
		{
			if (!com_dedicated->value)	// let dedicated servers continue after errors
			{
				common->Error("NET_SendPacket ERROR");
			}
		}
	}
}

//	tries to send an unreliable message to a connection, and handles the
// transmition / retransmition of the reliable messages.
//	Sends a message to a connection, fragmenting if necessary
//	A 0 length will still generate a packet and deal with the reliable messages.
void NET_SendPacket(netsrc_t sock, int length, const void* data, const netadr_t& to)
{
	if (GGameType & GAME_ET)
	{
		int packetloss, packetdelay;
		delaybuf_t** delaybuf_head, ** delaybuf_tail;

		switch (sock)
		{
		case NS_CLIENT:
			packetloss = cl_packetloss->integer;
			packetdelay = cl_packetdelay->integer;
			delaybuf_head = &cl_delaybuf_head;
			delaybuf_tail = &cl_delaybuf_tail;
			break;
		case NS_SERVER:
			packetloss = sv_packetloss->integer;
			packetdelay = sv_packetdelay->integer;
			delaybuf_head = &sv_delaybuf_head;
			delaybuf_tail = &sv_delaybuf_tail;
			break;
		}

		if (packetloss > 0)
		{
			if (((float)rand() / RAND_MAX) * 100 <= packetloss)
			{
				if (showpackets->integer)
				{
					common->Printf("drop packet %4i\n", length);
				}
				return;
			}
		}

		if (packetdelay)
		{
			int curtime;
			delaybuf_t* buf, * nextbuf;

			curtime = Sys_Milliseconds();

			//send any scheduled packets, starting from oldest
			for (buf = *delaybuf_head; buf; buf = nextbuf)
			{

				if ((buf->time + packetdelay) > curtime)
				{
					break;
				}

				if (showpackets->integer)
				{
					common->Printf("delayed packet(%dms) %4i\n", buf->time - curtime, buf->length);
				}

				NET_DoSendPacket(sock, buf->length, buf->data, buf->to);

				// remove from queue
				nextbuf = buf->next;
				*delaybuf_head = nextbuf;
				if (!*delaybuf_head)
				{
					*delaybuf_tail = NULL;
				}
				Mem_Free(buf);
			}

			// create buffer and add it to the queue
			buf = (delaybuf_t*)Mem_Alloc(sizeof(*buf));
			buf->sock = sock;
			buf->length = length;
			Com_Memcpy(buf->data, data, length);
			buf->to = to;
			buf->time = curtime;
			buf->next = NULL;

			if (*delaybuf_head)
			{
				(*delaybuf_tail)->next = buf;
			}
			else
			{
				*delaybuf_head = buf;
			}
			*delaybuf_tail = buf;

			return;
		}
	}

	NET_DoSendPacket(sock, length, data, to);
}

//	Sends a text message in an out-of-band datagram
void NET_OutOfBandPrint(netsrc_t sock, const netadr_t& adr, const char* format, ...)
{
	va_list argptr;
	char string[MAX_MSGLEN];

	// set the header
	string[0] = -1;
	string[1] = -1;
	string[2] = -1;
	string[3] = -1;

	va_start(argptr, format);
	Q_vsnprintf(string + 4, sizeof(string) - 4, format, argptr);
	va_end(argptr);

	// send the datagram
	NET_SendPacket(sock, String::Length(string), string, adr);
}

//	Sends a data message in an out-of-band datagram (only used for "connect")
void NET_OutOfBandData(netsrc_t sock, const netadr_t& adr, const byte* data, int length)
{
	byte buffer[MAX_MSGLEN_Q3 * 2];

	// set the header
	buffer[0] = 0xff;
	buffer[1] = 0xff;
	buffer[2] = 0xff;
	buffer[3] = 0xff;

	for (int i = 0; i < length; i++)
	{
		buffer[i + 4] = data[i];
	}

	QMsg mbuf;
	mbuf._data = buffer;
	mbuf.cursize = length + 4;
	Huff_Compress(&mbuf, 12);
	// send the datagram
	NET_SendPacket(sock, mbuf.cursize, mbuf._data, adr);
}

static bool Netchan_NeedReliable(netchan_t* chan)
{
	if (GGameType & GAME_Tech3)
	{
		return false;
	}

	// if the remote side dropped the last reliable message, resend it
	bool send_reliable = false;

	if (chan->incomingAcknowledged > chan->lastReliableSequence &&
		chan->incomingReliableAcknowledged != chan->outgoingReliableSequence)
	{
		send_reliable = true;
	}

	// if the reliable transmit buffer is empty, copy the current message out
	if (!chan->reliableOrUnsentLength && chan->message.cursize)
	{
		send_reliable = true;
	}

	return send_reliable;
}

//	Send one fragment of the current message
void Netchan_TransmitNextFragment(netchan_t* chan)
{
	QMsg send;
	byte send_buf[MAX_PACKETLEN];
	int fragmentLength;

	// write the packet header
	send.InitOOB(send_buf, sizeof(send_buf));					// <-- only do the oob here

	send.WriteLong(chan->outgoingSequence | FRAGMENT_BIT);

	// send the qport if we are a client
	if (chan->sock == NS_CLIENT)
	{
		send.WriteShort(qport->integer);
	}

	// copy the reliable message to the packet first
	fragmentLength = FRAGMENT_SIZE;
	if (chan->unsentFragmentStart  + fragmentLength > chan->reliableOrUnsentLength)
	{
		fragmentLength = chan->reliableOrUnsentLength - chan->unsentFragmentStart;
	}

	send.WriteShort(chan->unsentFragmentStart);
	send.WriteShort(fragmentLength);
	send.WriteData(chan->reliableOrUnsentBuffer + chan->unsentFragmentStart, fragmentLength);

	// send the datagram
	NET_SendPacket(chan->sock, send.cursize, send._data, chan->remoteAddress);

	if (showpackets->integer)
	{
		common->Printf("%s send %4i : s=%i fragment=%i,%i\n",
			netsrcString[chan->sock],
			send.cursize,
			chan->outgoingSequence,
			chan->unsentFragmentStart, fragmentLength);
	}

	chan->unsentFragmentStart += fragmentLength;

	// this exit condition is a little tricky, because a packet
	// that is exactly the fragment length still needs to send
	// a second packet of zero length so that the other side
	// can tell there aren't more to follow
	if (chan->unsentFragmentStart == chan->reliableOrUnsentLength && fragmentLength != FRAGMENT_SIZE)
	{
		chan->outgoingSequence++;
		chan->unsentFragments = false;
	}
}

void Netchan_Transmit(netchan_t* chan, int length, const byte* data)
{
	// check for message overflow
	if (!(GGameType & GAME_Tech3) && chan->message.overflowed)
	{
		common->Printf("%s:Outgoing message overflow\n", SOCK_AdrToString(chan->remoteAddress));
		return;
	}

	if (GGameType & GAME_Tech3 && length > (GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF))
	{
		common->Error("Netchan_Transmit: length = %i", length);
	}
	bool send_reliable = Netchan_NeedReliable(chan);

	if (!(GGameType & GAME_Tech3) && !chan->reliableOrUnsentLength && chan->message.cursize)
	{
		Com_Memcpy(chan->reliableOrUnsentBuffer, chan->messageBuffer, chan->message.cursize);
		chan->reliableOrUnsentLength = chan->message.cursize;
		chan->message.cursize = 0;
		chan->outgoingReliableSequence ^= 1;
	}

	chan->unsentFragmentStart = 0;

	// fragment large reliable messages
	if (GGameType & GAME_Tech3 && length >= FRAGMENT_SIZE)
	{
		chan->unsentFragments = true;
		chan->reliableOrUnsentLength = length;
		Com_Memcpy(chan->reliableOrUnsentBuffer, data, length);

		// only send the first fragment now
		Netchan_TransmitNextFragment(chan);

		return;
	}

	// write the packet header
	QMsg send;
	byte send_buf[MAX_MSGLEN_HW + PACKET_HEADER];
	send.InitOOB(send_buf, GGameType & GAME_QuakeWorld ? MAX_MSGLEN_QW + PACKET_HEADER :
		GGameType & GAME_HexenWorld ? MAX_MSGLEN_HW + PACKET_HEADER :
		GGameType & GAME_Quake2 ? MAX_MSGLEN_Q2 : MAX_PACKETLEN);

	if (GGameType & GAME_Tech3)
	{
		send.WriteLong(chan->outgoingSequence);
	}
	else
	{
		unsigned w1 = (chan->outgoingSequence & ~(1 << 31)) | (send_reliable << 31);
		unsigned w2 = (chan->incomingSequence & ~(1 << 31)) | (chan->incomingReliableSequence << 31);

		send.WriteLong(w1);
		send.WriteLong(w2);
	}

	chan->outgoingSequence++;

	// send the qport if we are a client
	if (!(GGameType & GAME_HexenWorld) && chan->sock == NS_CLIENT)
	{
		send.WriteShort(qport->integer);
	}

	// copy the reliable message to the packet first
	if (send_reliable)
	{
		send.WriteData(chan->reliableOrUnsentBuffer, chan->reliableOrUnsentLength);
		chan->lastReliableSequence = chan->outgoingSequence;
	}

	// add the unreliable part if space is available
	if (send.maxsize - send.cursize >= length)
	{
		send.WriteData(data, length);
	}
	else
	{
		common->Printf("Netchan_Transmit: dumped unreliable\n");
	}

	// send the datagram
	NET_SendPacket(chan->sock, send.cursize, send._data, chan->remoteAddress);

	if (showpackets->integer)
	{
		if (send_reliable)
		{
			common->Printf("%s send %4i : s=%i reliable=%i ack=%i rack=%i\n",
				netsrcString[chan->sock],
				send.cursize,
				chan->outgoingSequence - 1,
				chan->outgoingReliableSequence,
				chan->incomingSequence,
				chan->incomingReliableSequence);
		}
		else
		{
			common->Printf("%s send %4i : s=%i ack=%i rack=%i\n",
				netsrcString[chan->sock],
				send.cursize,
				chan->outgoingSequence - 1,
				chan->incomingSequence,
				chan->incomingReliableSequence);
		}
	}
}

//	Returns true if the last reliable message has acked
bool Netchan_CanReliable(netchan_t* chan)
{
	if (chan->reliableOrUnsentLength)
	{
		return false;			// waiting for ack
	}
	return true;
}

//	Returns false if the message should not be processed due to being
// out of order or a fragment.
//	Msg must be large enough to hold MAX_MSGLEN_Q3, because if this is the
// final fragment of a multi-part message, the entire thing will be
// copied out.
bool Netchan_Process(netchan_t* chan, QMsg* msg)
{
	// get sequence numbers
	msg->BeginReadingOOB();
	int sequence = msg->ReadLong();

	int sequence_ack = 0;
	bool reliable_message = false;
	int reliable_ack = 0;
	bool fragmented = false;
	if (GGameType & GAME_Tech3)
	{
		if (sequence & FRAGMENT_BIT)
		{
			fragmented = true;
		}
	}
	else
	{
		sequence_ack = msg->ReadLong();

		reliable_message = (unsigned)sequence >> 31;
		reliable_ack = (unsigned)sequence_ack >> 31;

		sequence_ack &= ~(1 << 31);
	}
	sequence &= ~(1 << 31);

	// read the qport if we are a server
	if (!(GGameType & GAME_HexenWorld) && chan->sock == NS_SERVER)
	{
		msg->ReadShort();
	}

	int fragmentStart, fragmentLength;
	// read the fragment information
	if (fragmented)
	{
		fragmentStart = msg->ReadShort();
		fragmentLength = msg->ReadShort();
	}
	else
	{
		fragmentStart = 0;		// stop warning message
		fragmentLength = 0;
	}

	if (showpackets->integer)
	{
		if (reliable_message)
		{
			common->Printf("recv %4i : s=%i reliable=%i ack=%i rack=%i\n",
				msg->cursize,
				sequence,
				chan->incomingReliableSequence ^ 1,
				sequence_ack,
				reliable_ack);
		}
		else if (fragmented)
		{
			common->Printf("%s recv %4i : s=%i fragment=%i,%i\n",
				netsrcString[chan->sock],
				msg->cursize,
				sequence,
				fragmentStart, fragmentLength);
		}
		else
		{
			common->Printf("%s recv %4i : s=%i ack=%i rack=%i\n",
				netsrcString[chan->sock],
				msg->cursize,
				sequence,
				sequence_ack,
				reliable_ack);
		}
	}

	//
	// discard out of order or duplicated packets
	//
	if (sequence <= chan->incomingSequence)
	{
		if (showdrop->integer || showpackets->integer)
		{
			common->Printf("%s:Out of order packet %i at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				sequence,
				chan->incomingSequence);
		}
		return false;
	}

	//
	// dropped packets don't keep the message from being used
	//
	chan->dropped = sequence - (chan->incomingSequence + 1);
	if (chan->dropped > 0)
	{
		chan->dropCount += 1;

		if (showdrop->integer || showpackets->integer)
		{
			common->Printf("%s:Dropped %i packets at %i\n",
				SOCK_AdrToString(chan->remoteAddress),
				chan->dropped,
				sequence);
		}
	}

	//
	// if this is the final framgent of a reliable message,
	// bump incoming_reliable_sequence
	//
	if (fragmented)
	{
		// TTimo
		// make sure we add the fragments in correct order
		// either a packet was dropped, or we received this one too soon
		// we don't reconstruct the fragments. we will wait till this fragment gets to us again
		// (NOTE: we could probably try to rebuild by out of order chunks if needed)
		if (sequence != chan->fragmentSequence)
		{
			chan->fragmentSequence = sequence;
			chan->fragmentLength = 0;
		}

		// if we missed a fragment, dump the message
		if (fragmentStart != chan->fragmentLength)
		{
			if (showdrop->integer || showpackets->integer)
			{
				common->Printf("%s:Dropped a message fragment, sequence %d\n",
					SOCK_AdrToString(chan->remoteAddress),
					sequence);
			}
			// we can still keep the part that we have so far,
			// so we don't need to clear chan->fragmentLength
			return false;
		}

		// copy the fragment to the fragment buffer
		if (fragmentLength < 0 || msg->readcount + fragmentLength > msg->cursize ||
			chan->fragmentLength + fragmentLength > (GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF))
		{
			if (showdrop->integer || showpackets->integer)
			{
				common->Printf("%s:illegal fragment length\n",
					SOCK_AdrToString(chan->remoteAddress));
			}
			return false;
		}

		Com_Memcpy(chan->fragmentBuffer + chan->fragmentLength,
			msg->_data + msg->readcount, fragmentLength);

		chan->fragmentLength += fragmentLength;

		// if this wasn't the last fragment, don't process anything
		if (fragmentLength == FRAGMENT_SIZE)
		{
			return false;
		}

		if (chan->fragmentLength > msg->maxsize)
		{
			common->Printf("%s:fragmentLength %i > msg->maxsize\n",
				SOCK_AdrToString(chan->remoteAddress),
				chan->fragmentLength);
			return false;
		}

		// copy the full message over the partial fragment

		// make sure the sequence number is still there
		*(int*)msg->_data = LittleLong(sequence);

		Com_Memcpy(msg->_data + 4, chan->fragmentBuffer, chan->fragmentLength);
		msg->cursize = chan->fragmentLength + 4;
		chan->fragmentLength = 0;
		msg->readcount = 4;	// past the sequence number
		msg->bit = 32;	// past the sequence number

		// TTimo
		// clients were not acking fragmented messages
		chan->incomingSequence = sequence;

		return true;
	}

	//
	// the message can now be read from the current message pointer
	//
	chan->incomingSequence = sequence;

	if (!(GGameType & GAME_Tech3))
	{
		//
		// if the current outgoing reliable message has been acknowledged
		// clear the buffer to make way for the next
		//
		if (reliable_ack == chan->outgoingReliableSequence)
		{
			chan->reliableOrUnsentLength = 0;	// it has been received

		}
		//
		// if this message contains a reliable message, bump incoming_reliable_sequence
		//
		chan->incomingAcknowledged = sequence_ack;
		chan->incomingReliableAcknowledged = reliable_ack;
		if (reliable_message)
		{
			chan->incomingReliableSequence ^= 1;
		}
	}

	return true;
}
