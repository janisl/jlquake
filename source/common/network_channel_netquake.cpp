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

// This is the network info/connection protocol.  It is used to find Quake
// servers, get info about them, and connect to them.  Once connected, the
// Quake game protocol (documented elsewhere) is used.
//
//
// General notes:
//	game_name is currently always "QUAKE", but is there so this same protocol
//		can be used for future games as well; can you say Quake2?
//
// CCREQ_CONNECT
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	Q1NET_PROTOCOL_VERSION
//
// CCREQ_SERVER_INFO
//		string	game_name				"QUAKE"
//		byte	net_protocol_version	Q1NET_PROTOCOL_VERSION
//
// CCREQ_PLAYER_INFO
//		byte	player_number
//
// CCREQ_RULE_INFO
//		string	rule
//
//
//
// CCREP_ACCEPT
//		long	port
//
// CCREP_REJECT
//		string	reason
//
// CCREP_SERVER_INFO
//		string	server_address
//		string	host_name
//		string	level_name
//		byte	current_players
//		byte	max_players
//		byte	protocol_version	Q1NET_PROTOCOL_VERSION
//
// CCREP_PLAYER_INFO
//		byte	player_number
//		string	name
//		long	colors
//		long	frags
//		long	connect_time
//		string	address
//
// CCREP_RULE_INFO
//		string	rule
//		string	value

//	note:
//		There are two address forms used above.  The short form is just a
//		port number.  The address that goes along with the port is defined as
//		"whatever address you receive this reponse from".  This lets us use
//		the host OS to solve the problem of multiple host addresses (possibly
//		with no routing between them); the host will use the right address
//		when we reply to the inbound connection request.  The long from is
//		a full address and port in a string.  It is used for returning the
//		address of a server that is not running locally.

#include "qcommon.h"

int net_hostport;

qsocket_t* net_activeSockets = NULL;
qsocket_t* net_freeSockets = NULL;
int net_numsockets = 0;

double net_time;

int net_activeconnections = 0;
int hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

bool localconnectpending = false;
qsocket_t* loop_client = NULL;
qsocket_t* loop_server = NULL;

bool tcpipAvailable = false;

static const char* net_interface;
int net_controlsocket;
int net_acceptsocket = -1;		// socket for fielding new connections

packetBuffer_t packetBuffer;

/* statistic counters */
int packetsSent = 0;
int packetsReSent = 0;
int packetsReceived = 0;
int receivedDuplicateCount = 0;
int shortPacketCount = 0;
int droppedDatagrams;
int messagesSent = 0;
int messagesReceived = 0;
int unreliableMessagesSent = 0;
int unreliableMessagesReceived = 0;

netadr_t banAddr;
netadr_t banMask;

int udp_controlSock;
bool udp_initialized;

//	Called by drivers when a new communications endpoint is required
// The sequence and buffer fields will be filled in properly
qsocket_t* NET_NewQSocket()
{
	if (net_freeSockets == NULL)
	{
		return NULL;
	}

	// get one from free list
	qsocket_t* sock = net_freeSockets;
	net_freeSockets = sock->next;

	// add it to active list
	sock->next = net_activeSockets;
	net_activeSockets = sock;

	sock->disconnected = false;
	sock->connecttime = net_time;
	String::Cpy(sock->address,"UNSET ADDRESS");
	sock->socket = 0;
	sock->canSend = true;
	sock->sendNext = false;

	return sock;
}

void NET_FreeQSocket(qsocket_t* sock)
{
	// remove it from active list
	if (sock == net_activeSockets)
	{
		net_activeSockets = net_activeSockets->next;
	}
	else
	{
		qsocket_t* s;
		for (s = net_activeSockets; s; s = s->next)
		{
			if (s->next == sock)
			{
				s->next = sock->next;
				break;
			}
		}
		if (!s)
		{
			common->FatalError("NET_FreeQSocket: not active\n");
		}
	}

	// add it to free list
	sock->next = net_freeSockets;
	net_freeSockets = sock;
	sock->disconnected = true;
}

double SetNetTime()
{
	net_time = Sys_DoubleTime();
	return net_time;
}

qsocket_t* Loop_Connect(const char* host, netchan_t* chan)
{
	if (String::Cmp(host,"local") != 0)
	{
		return NULL;
	}

	localconnectpending = true;

	if (!loop_client)
	{
		if ((loop_client = NET_NewQSocket()) == NULL)
		{
			common->Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_client->address, "localhost");
	}
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loopbacks[0].canSend = true;

	if (!loop_server)
	{
		if ((loop_server = NET_NewQSocket()) == NULL)
		{
			common->Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_server->address, "LOCAL");
	}
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loopbacks[1].canSend = true;

	chan->remoteAddress.type = NA_LOOPBACK;
	return loop_client;
}

qsocket_t* Loop_CheckNewConnections(netadr_t* outaddr)
{
	if (!localconnectpending)
	{
		return NULL;
	}

	localconnectpending = false;
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loopbacks[1].canSend = true;
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loopbacks[0].canSend = true;
	outaddr->type = NA_LOOPBACK;
	return loop_server;
}

int Loop_GetMessage(netchan_t* chan, QMsg* message)
{
	netadr_t net_from;
	int ret = NET_GetLoopPacket(chan->sock, &net_from, message);

	if (ret == 1)
	{
		loopbacks[chan->sock ^ 1].canSend = true;
	}

	return ret;
}

int Loop_SendMessage(netchan_t* chan, QMsg* data)
{
	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		common->Error("Loop_SendMessage: overflow\n");
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 1);

	loopbacks[chan->sock].canSend = false;
	return 1;
}

int Loop_SendUnreliableMessage(netchan_t* chan, QMsg* data)
{
	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		return 0;
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 2);
	return 1;
}

bool Loop_CanSendMessage(netchan_t* chan)
{
	return loopbacks[chan->sock].canSend;
}

void Loop_Close(qsocket_t* sock, netchan_t* chan)
{
	loopbacks[chan->sock].send = 0;
	loopbacks[chan->sock].get = 0;
	loopbacks[chan->sock].canSend = true;
	if (sock == loop_client)
	{
		loop_client = NULL;
	}
	else
	{
		loop_server = NULL;
	}
}

int UDPNQ_OpenSocket(int port)
{
	int newsocket = SOCK_Open(net_interface, port);
	if (newsocket == 0)
	{
		return -1;
	}
	return newsocket;
}

int UDP_Init()
{
	if (COM_CheckParm("-noudp"))
	{
		return -1;
	}

	if (!SOCK_Init())
	{
		return -1;
	}

	// determine my name & address
	SOCK_GetLocalAddress();

	int i = COM_CheckParm("-ip");
	if (i)
	{
		if (i < COM_Argc() - 1)
		{
			net_interface = COM_Argv(i + 1);
		}
		else
		{
			common->FatalError("NET_Init: you must specify an IP address after -ip");
		}
	}

	if ((net_controlsocket = UDPNQ_OpenSocket(PORT_ANY)) == -1)
	{
		common->Printf("UDP_Init: Unable to open control socket\n");
		SOCK_Shutdown();
		return -1;
	}

	tcpipAvailable = true;

	return net_controlsocket;
}

int UDP_CloseSocket(int socket)
{
	SOCK_Close(socket);
	return 0;
}

void UDP_Listen(bool state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
		{
			return;
		}
		if ((net_acceptsocket = UDPNQ_OpenSocket(net_hostport)) == -1)
		{
			common->FatalError("UDP_Listen: Unable to open accept socket\n");
		}
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
	{
		return;
	}
	UDP_CloseSocket(net_acceptsocket);
	net_acceptsocket = -1;
}

void UDP_Shutdown()
{
	UDP_Listen(false);
	UDP_CloseSocket(net_controlsocket);
	SOCK_Shutdown();
}

int UDP_Read(int socket, byte* buf, int len, netadr_t* addr)
{
	int ret = SOCK_Recv(socket, buf, len, addr);
	if (ret == SOCKRECV_NO_DATA)
	{
		return 0;
	}
	if (ret == SOCKRECV_ERROR)
	{
		return -1;
	}
	return ret;
}

int UDP_Write(int socket, byte* buf, int len, netadr_t* addr)
{
	int ret = SOCK_Send(socket, buf, len, addr);
	if (ret == SOCKSEND_WOULDBLOCK)
	{
		return 0;
	}
	return ret;
}

int UDP_Broadcast(int socket, byte* buf, int len)
{
	netadr_t to;
	Com_Memset(&to, 0, sizeof(to));
	to.type = NA_BROADCAST;
	to.port = BigShort(net_hostport);
	int ret = SOCK_Send(socket, buf, len, &to);
	if (ret == SOCKSEND_WOULDBLOCK)
	{
		return 0;
	}
	return ret;
}

int UDP_GetAddrFromName(const char* name, netadr_t* addr)
{
	if (!SOCK_StringToAdr(name, addr, 0))
	{
		return -1;
	}

	addr->port = BigShort(net_hostport);

	return 0;
}

int UDP_AddrCompare(netadr_t* addr1, netadr_t* addr2)
{
	if (SOCK_CompareAdr(*addr1, *addr2))
	{
		return 0;
	}

	if (SOCK_CompareBaseAdr(*addr1, *addr2))
	{
		return 1;
	}

	return -1;
}

int UDP_GetNameFromAddr(netadr_t* addr, char* name)
{
	const char* host = SOCK_GetHostByAddr(addr);
	if (host)
	{
		String::NCpy(name, host, NET_NAMELEN_Q1 - 1);
		return 0;
	}

	String::Cpy(name, SOCK_AdrToString(*addr));
	return 0;
}

int Datagram_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
#ifdef DEBUG
	if (data->cursize == 0)
	{
		Sys_Error("Datagram_SendMessage: zero length message\n");
	}

	if (data->cursize > GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_MSGLEN_Q1)
	{
		Sys_Error("Datagram_SendMessage: message too big %u\n", data->cursize);
	}

	if (sock->canSend == false)
	{
		Sys_Error("SendMessage: called with canSend == false\n");
	}
#endif

	Com_Memcpy(chan->reliableOrUnsentBuffer, data->_data, data->cursize);
	chan->reliableOrUnsentLength = data->cursize;

	unsigned int dataLen;
	unsigned int eom;
	if (data->cursize <= MAX_DATAGRAM_QH)
	{
		dataLen = data->cursize;
		eom = NETFLAG_EOM;
	}
	else
	{
		dataLen = MAX_DATAGRAM_QH;
		eom = 0;
	}
	unsigned int packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(chan->outgoingReliableSequence++);
	Com_Memcpy(packetBuffer.data, chan->reliableOrUnsentBuffer, dataLen);

	sock->canSend = false;

	if (UDP_Write(sock->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	chan->lastSent = net_time * 1000;
	packetsSent++;
	return 1;
}

int SendMessageNext(qsocket_t* sock, netchan_t* chan)
{
	unsigned int dataLen;
	unsigned int eom;
	if (chan->reliableOrUnsentLength <= MAX_DATAGRAM_QH)
	{
		dataLen = chan->reliableOrUnsentLength;
		eom = NETFLAG_EOM;
	}
	else
	{
		dataLen = MAX_DATAGRAM_QH;
		eom = 0;
	}
	unsigned int packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(chan->outgoingReliableSequence++);
	Com_Memcpy(packetBuffer.data, chan->reliableOrUnsentBuffer, dataLen);

	sock->sendNext = false;

	if (UDP_Write(sock->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	chan->lastSent = net_time * 1000;
	packetsSent++;
	return 1;
}

int ReSendMessage(qsocket_t* sock, netchan_t* chan)
{
	unsigned int dataLen;
	unsigned int eom;
	if (chan->reliableOrUnsentLength <= MAX_DATAGRAM_QH)
	{
		dataLen = chan->reliableOrUnsentLength;
		eom = NETFLAG_EOM;
	}
	else
	{
		dataLen = MAX_DATAGRAM_QH;
		eom = 0;
	}
	unsigned int packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(chan->outgoingReliableSequence - 1);
	Com_Memcpy(packetBuffer.data, chan->reliableOrUnsentBuffer, dataLen);

	sock->sendNext = false;

	if (UDP_Write(sock->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	chan->lastSent = net_time * 1000;
	packetsReSent++;
	return 1;
}

bool Datagram_CanSendMessage(qsocket_t* sock, netchan_t* chan)
{
	if (sock->sendNext)
	{
		SendMessageNext(sock, chan);
	}

	return sock->canSend;
}

int Datagram_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
#ifdef DEBUG
	if (data->cursize == 0)
	{
		Sys_Error("Datagram_SendUnreliableMessage: zero length message\n");
	}

	if (data->cursize > MAX_DATAGRAM_QH)
	{
		Sys_Error("Datagram_SendUnreliableMessage: message too big %u\n", data->cursize);
	}
#endif

	int packetLen = NET_HEADERSIZE + data->cursize;

	packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
	packetBuffer.sequence = BigLong(chan->outgoingSequence++);
	Com_Memcpy(packetBuffer.data, data->_data, data->cursize);

	if (UDP_Write(sock->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	packetsSent++;
	return 1;
}

int Datagram_GetMessage(qsocket_t* sock, netchan_t* chan, QMsg* message)
{
	int length;
	unsigned int flags;
	int ret = 0;
	netadr_t readaddr;
	unsigned int count;

	if (!sock->canSend)
	{
		if ((net_time * 1000 - chan->lastSent) > 1000)
		{
			ReSendMessage(sock, chan);
		}
	}

	while (1)
	{
		length = UDP_Read(sock->socket, (byte*)&packetBuffer, NET_DATAGRAMSIZE, &readaddr);

//	if ((rand() & 255) > 220)
//		continue;

		if (length == 0)
		{
			break;
		}

		if (length == -1)
		{
			common->Printf("Read error\n");
			return -1;
		}

		if (UDP_AddrCompare(&readaddr, &chan->remoteAddress) != 0)
		{
#ifdef DEBUG
			common->DPrintf("Forged packet received\n");
			common->DPrintf("Expected: %s\n", SOCK_AdrToString(sock->addr));
			common->DPrintf("Received: %s\n", SOCK_AdrToString(readaddr));
#endif
			continue;
		}

		if (length < (int)NET_HEADERSIZE)
		{
			shortPacketCount++;
			continue;
		}

		length = BigLong(packetBuffer.length);
		flags = length & (~NETFLAG_LENGTH_MASK);
		length &= NETFLAG_LENGTH_MASK;

		if (flags & NETFLAG_CTL)
		{
			continue;
		}

		int sequence = BigLong(packetBuffer.sequence);
		packetsReceived++;

		if (flags & NETFLAG_UNRELIABLE)
		{
			if (sequence < chan->incomingSequence)
			{
				common->DPrintf("Got a stale datagram\n");
				ret = 0;
				break;
			}
			if (sequence != chan->incomingSequence)
			{
				count = sequence - chan->incomingSequence;
				droppedDatagrams += count;
				common->DPrintf("Dropped %u datagram(s)\n", count);
			}
			chan->incomingSequence = sequence + 1;

			length -= NET_HEADERSIZE;

			message->Clear();
			message->WriteData(packetBuffer.data, length);

			ret = 2;
			break;
		}

		if (flags & NETFLAG_ACK)
		{
			if (sequence != (chan->outgoingReliableSequence - 1))
			{
				common->DPrintf("Stale ACK received\n");
				continue;
			}
			if (sequence == chan->incomingReliableAcknowledged)
			{
				chan->incomingReliableAcknowledged++;
				if (chan->incomingReliableAcknowledged != chan->outgoingReliableSequence)
				{
					common->DPrintf("ack sequencing error\n");
				}
			}
			else
			{
				common->DPrintf("Duplicate ACK received\n");
				continue;
			}
			chan->reliableOrUnsentLength -= MAX_DATAGRAM_QH;
			if (chan->reliableOrUnsentLength > 0)
			{
				Com_Memcpy(chan->reliableOrUnsentBuffer, chan->reliableOrUnsentBuffer + MAX_DATAGRAM_QH, chan->reliableOrUnsentLength);
				sock->sendNext = true;
			}
			else
			{
				chan->reliableOrUnsentLength = 0;
				sock->canSend = true;
			}
			continue;
		}

		if (flags & NETFLAG_DATA)
		{
			packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
			packetBuffer.sequence = BigLong(sequence);
			UDP_Write(sock->socket, (byte*)&packetBuffer, NET_HEADERSIZE, &readaddr);

			if (sequence != chan->incomingReliableSequence)
			{
				receivedDuplicateCount++;
				continue;
			}
			chan->incomingReliableSequence++;

			length -= NET_HEADERSIZE;

			if (flags & NETFLAG_EOM)
			{
				message->Clear();
				message->WriteData(chan->fragmentBuffer, chan->fragmentLength);
				message->WriteData(packetBuffer.data, length);
				chan->fragmentLength = 0;

				ret = 1;
				break;
			}

			Com_Memcpy(chan->fragmentBuffer + chan->fragmentLength, packetBuffer.data, length);
			chan->fragmentLength += length;
			continue;
		}
	}

	if (sock->sendNext)
	{
		SendMessageNext(sock, chan);
	}

	return ret;
}

static void PrintStats(qsocket_t* s)
{
	common->Printf("canSend = %4u   \n", s->canSend);
//	common->Printf("sendSeq = %4u   ", s->outgoingReliableSequence);
//	common->Printf("recvSeq = %4u   \n", s->incomingReliableSequence);
	common->Printf("\n");
}

void NET_Stats_f()
{
	qsocket_t* s;

	if (Cmd_Argc() == 1)
	{
		common->Printf("unreliable messages sent   = %i\n", unreliableMessagesSent);
		common->Printf("unreliable messages recv   = %i\n", unreliableMessagesReceived);
		common->Printf("reliable messages sent     = %i\n", messagesSent);
		common->Printf("reliable messages received = %i\n", messagesReceived);
		common->Printf("packetsSent                = %i\n", packetsSent);
		common->Printf("packetsReSent              = %i\n", packetsReSent);
		common->Printf("packetsReceived            = %i\n", packetsReceived);
		common->Printf("receivedDuplicateCount     = %i\n", receivedDuplicateCount);
		common->Printf("shortPacketCount           = %i\n", shortPacketCount);
		common->Printf("droppedDatagrams           = %i\n", droppedDatagrams);
	}
	else if (String::Cmp(Cmd_Argv(1), "*") == 0)
	{
		for (s = net_activeSockets; s; s = s->next)
			PrintStats(s);
		for (s = net_freeSockets; s; s = s->next)
			PrintStats(s);
	}
	else
	{
		for (s = net_activeSockets; s; s = s->next)
			if (String::ICmp(Cmd_Argv(1), s->address) == 0)
			{
				break;
			}
		if (s == NULL)
		{
			for (s = net_freeSockets; s; s = s->next)
				if (String::ICmp(Cmd_Argv(1), s->address) == 0)
				{
					break;
				}
		}
		if (s == NULL)
		{
			return;
		}
		PrintStats(s);
	}
}

int Datagram_Init()
{
	Cmd_AddCommand("net_stats", NET_Stats_f);

	if (COM_CheckParm("-nolan"))
	{
		return -1;
	}

	int csock = UDP_Init();
	if (csock != -1)
	{
		udp_initialized = true;
		udp_controlSock = csock;
	}

	Cmd_AddCommand("ban", NET_Ban_f);

	return 0;
}

void Datagram_Shutdown()
{
	if (udp_initialized)
	{
		UDP_Shutdown();
		udp_initialized = false;
	}
}

void Datagram_Close(qsocket_t* sock, netchan_t* chan)
{
	UDPNQ_OpenSocket(sock->socket);
}

void Datagram_Listen(bool state)
{
	if (udp_initialized)
	{
		UDP_Listen(state);
	}
}

void NET_Close(qsocket_t* sock, netchan_t* chan)
{
	if (!sock)
	{
		return;
	}

	if (sock->disconnected)
	{
		return;
	}

	SetNetTime();

	// call the driver_Close function
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		Loop_Close(sock, chan);
	}
	else
	{
		Datagram_Close(sock, chan);
	}

	NET_FreeQSocket(sock);
}

void Datagram_SearchForHosts(bool xmit)
{
	if (!udp_initialized)
	{
		return;
	}
	int ret;
	int n;
	int i;
	netadr_t readaddr;
	int control;

	QMsg net_message;
	byte net_message_buf[MAX_DATAGRAM_QH];
	net_message.InitOOB(net_message_buf, MAX_DATAGRAM_QH);

	if (xmit)
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREQ_SERVER_INFO);
		net_message.WriteString2(GGameType & GAME_Hexen2 ? NET_NAME_ID : "QUAKE");
		net_message.WriteByte(GGameType & GAME_Hexen2 ? H2NET_PROTOCOL_VERSION : Q1NET_PROTOCOL_VERSION);
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Broadcast(udp_controlSock, net_message._data, net_message.cursize);
		net_message.Clear();
	}

	while ((ret = UDP_Read(udp_controlSock, net_message._data, net_message.maxsize, &readaddr)) > 0)
	{
		if (ret < (int)sizeof(int))
		{
			continue;
		}
		net_message.cursize = ret;

		// is the cache full?
		if (hostCacheCount == HOSTCACHESIZE)
		{
			continue;
		}

		net_message.BeginReadingOOB();
		control = BigLong(*((int*)net_message._data));
		net_message.ReadLong();
		if (control == -1)
		{
			continue;
		}
		if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL)
		{
			continue;
		}
		if ((control & NETFLAG_LENGTH_MASK) != ret)
		{
			continue;
		}

		if (net_message.ReadByte() != CCREP_SERVER_INFO)
		{
			continue;
		}

		//	This is server's IP, or more exatly what server thinks is it's IP.
		// Originally this string was converted into readaddr and that address
		// was then saved in host cache. The whole thin is stupid as server
		// can't know exatly which IP this client sees it by and secondly
		// readaddr already contains exactly the right address.
		net_message.ReadString2();

		// search the cache for this server
		for (n = 0; n < hostCacheCount; n++)
		{
			if (UDP_AddrCompare(&readaddr, &hostcache[n].addr) == 0)
			{
				break;
			}
		}

		// is it already there?
		if (n < hostCacheCount)
		{
			continue;
		}

		// add it
		hostCacheCount++;
		String::Cpy(hostcache[n].name, net_message.ReadString2());
		String::Cpy(hostcache[n].map, net_message.ReadString2());
		hostcache[n].users = net_message.ReadByte();
		hostcache[n].maxusers = net_message.ReadByte();
		if (net_message.ReadByte() != (GGameType & GAME_Hexen2 ? H2NET_PROTOCOL_VERSION : Q1NET_PROTOCOL_VERSION))
		{
			String::Cpy(hostcache[n].cname, hostcache[n].name);
			hostcache[n].cname[14] = 0;
			String::Cpy(hostcache[n].name, "*");
			String::Cat(hostcache[n].name, sizeof(hostcache[n].name), hostcache[n].cname);
		}
		hostcache[n].addr = readaddr;
		hostcache[n].driver = 1;
		String::Cpy(hostcache[n].cname, SOCK_AdrToString(readaddr));

		// check for a name conflict
		for (i = 0; i < hostCacheCount; i++)
		{
			if (i == n)
			{
				continue;
			}
			if (String::ICmp(hostcache[n].name, hostcache[i].name) == 0)
			{
				i = String::Length(hostcache[n].name);
				if (i < 15 && hostcache[n].name[i - 1] > '8')
				{
					hostcache[n].name[i] = '0';
					hostcache[n].name[i + 1] = 0;
				}
				else
				{
					hostcache[n].name[i - 1]++;
				}
				i = -1;
			}
		}
	}
}
