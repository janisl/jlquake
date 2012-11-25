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
#include "../server/public.h"

struct packetBuffer_t
{
	unsigned int length;
	unsigned int sequence;
	byte data[MAX_DATAGRAM_QH];
};

int net_hostport;
int DEFAULTnet_hostport;

double net_time;

int net_activeconnections = 0;
int hostCacheCount = 0;
hostcache_t hostcache[HOSTCACHESIZE];

static bool localconnectpending = false;

bool tcpipAvailable = false;

static const char* net_interface;
static int net_controlsocket;
int net_acceptsocket = -1;		// socket for fielding new connections

static packetBuffer_t packetBuffer;

/* statistic counters */
static int packetsSent = 0;
static int packetsReSent = 0;
static int packetsReceived = 0;
static int receivedDuplicateCount = 0;
static int shortPacketCount = 0;
static int droppedDatagrams;
static int messagesSent = 0;
static int messagesReceived = 0;
static int unreliableMessagesSent = 0;
static int unreliableMessagesReceived = 0;

netadr_t banAddr;
netadr_t banMask;

int udp_controlSock;
bool udp_initialized;

static Cvar* net_messagetimeout;

bool datagram_initialized;
bool net_listening = false;

double SetNetTime()
{
	net_time = Sys_DoubleTime();
	return net_time;
}

bool Loop_Connect(const char* host, netchan_t* chan)
{
	if (String::Cmp(host,"local") != 0)
	{
		return false;
	}

	localconnectpending = true;

	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loopbacks[0].canSend = true;

	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loopbacks[1].canSend = true;

	chan->remoteAddress.type = NA_LOOPBACK;
	return true;
}

bool Loop_CheckNewConnections(netadr_t* outaddr)
{
	if (!localconnectpending)
	{
		return false;
	}

	localconnectpending = false;
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loopbacks[1].canSend = true;
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loopbacks[0].canSend = true;
	outaddr->type = NA_LOOPBACK;
	return true;
}

static int Loop_GetMessage(netchan_t* chan, QMsg* message)
{
	netadr_t net_from;
	int ret = NET_GetLoopPacket(chan->sock, &net_from, message);

	if (ret == 1)
	{
		loopbacks[chan->sock ^ 1].canSend = true;
	}

	return ret;
}

static int Loop_SendMessage(netchan_t* chan, QMsg* data)
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

static int Loop_SendUnreliableMessage(netchan_t* chan, QMsg* data)
{
	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		return 0;
	}

	NET_SendLoopPacket(chan->sock, data->cursize, data->_data, 2);
	return 1;
}

static bool Loop_CanSendMessage(netchan_t* chan)
{
	return loopbacks[chan->sock].canSend;
}

static void Loop_Close(netchan_t* chan)
{
	loopbacks[chan->sock].send = 0;
	loopbacks[chan->sock].get = 0;
	loopbacks[chan->sock].canSend = true;
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

static int UDP_Init()
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

static void UDP_Listen(bool state)
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

static void UDP_Shutdown()
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
	int ret = SOCK_Send(socket, buf, len, *addr);
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
	int ret = SOCK_Send(socket, buf, len, to);
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

static int Datagram_SendMessage(netchan_t* chan, QMsg* data)
{
#ifdef DEBUG
	if (data->cursize == 0)
	{
		common->FatalError("Datagram_SendMessage: zero length message\n");
	}

	if (data->cursize > GGameType & GAME_Hexen2 ? MAX_MSGLEN_H2 : MAX_MSGLEN_Q1)
	{
		common->FatalError("Datagram_SendMessage: message too big %u\n", data->cursize);
	}

	if (sock->canSend == false)
	{
		common->FatalError("SendMessage: called with canSend == false\n");
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

	chan->canSend = false;

	if (UDP_Write(chan->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	chan->lastSent = net_time * 1000;
	packetsSent++;
	return 1;
}

static int SendMessageNext(netchan_t* chan)
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

	chan->sendNext = false;

	if (UDP_Write(chan->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	chan->lastSent = net_time * 1000;
	packetsSent++;
	return 1;
}

static int ReSendMessage(netchan_t* chan)
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

	chan->sendNext = false;

	if (UDP_Write(chan->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	chan->lastSent = net_time * 1000;
	packetsReSent++;
	return 1;
}

static bool Datagram_CanSendMessage(netchan_t* chan)
{
	if (chan->sendNext)
	{
		SendMessageNext(chan);
	}

	return chan->canSend;
}

static int Datagram_SendUnreliableMessage(netchan_t* chan, QMsg* data)
{
#ifdef DEBUG
	if (data->cursize == 0)
	{
		common->FatalError("Datagram_SendUnreliableMessage: zero length message\n");
	}

	if (data->cursize > MAX_DATAGRAM_QH)
	{
		common->FatalError("Datagram_SendUnreliableMessage: message too big %u\n", data->cursize);
	}
#endif

	int packetLen = NET_HEADERSIZE + data->cursize;

	packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
	packetBuffer.sequence = BigLong(chan->outgoingSequence++);
	Com_Memcpy(packetBuffer.data, data->_data, data->cursize);

	if (UDP_Write(chan->socket, (byte*)&packetBuffer, packetLen, &chan->remoteAddress) == -1)
	{
		return -1;
	}

	packetsSent++;
	return 1;
}

static int Datagram_GetMessage(netchan_t* chan, QMsg* message)
{
	int length;
	unsigned int flags;
	int ret = 0;
	netadr_t readaddr;
	unsigned int count;

	if (!chan->canSend)
	{
		if ((net_time * 1000 - chan->lastSent) > 1000)
		{
			ReSendMessage(chan);
		}
	}

	while (1)
	{
		length = UDP_Read(chan->socket, (byte*)&packetBuffer, NET_DATAGRAMSIZE, &readaddr);

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
			common->DPrintf("Expected: %s\n", SOCK_AdrToString(chan->remoteAddress));
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
				chan->sendNext = true;
			}
			else
			{
				chan->reliableOrUnsentLength = 0;
				chan->canSend = true;
			}
			continue;
		}

		if (flags & NETFLAG_DATA)
		{
			packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
			packetBuffer.sequence = BigLong(sequence);
			UDP_Write(chan->socket, (byte*)&packetBuffer, NET_HEADERSIZE, &readaddr);

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

	if (chan->sendNext)
	{
		SendMessageNext(chan);
	}

	return ret;
}

static void NET_Stats_f()
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

static int Datagram_Init()
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

static void Datagram_Shutdown()
{
	if (udp_initialized)
	{
		UDP_Shutdown();
		udp_initialized = false;
	}
}

static void Datagram_Close(netchan_t* chan)
{
	UDP_CloseSocket(chan->socket);
	chan->socket = 0;
}

static void Datagram_Listen(bool state)
{
	if (udp_initialized)
	{
		UDP_Listen(state);
	}
}

void NET_Close(netchan_t* chan)
{
	if (!chan)
	{
		return;
	}

	if (chan->disconnected)
	{
		return;
	}

	SetNetTime();

	// call the driver_Close function
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		Loop_Close(chan);
	}
	else
	{
		Datagram_Close(chan);
	}

	chan->disconnected = true;
}

//	If there is a complete message, return it in net_message
//	returns 0 if no data is waiting
//	returns 1 if a message was received
//	returns -1 if connection is invalid
int NET_GetMessage(netchan_t* chan, QMsg* message)
{
	if (!chan)
	{
		return -1;
	}

	if (chan->disconnected)
	{
		common->Printf("NET_GetMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();

	int ret;
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		ret = Loop_GetMessage(chan, message);
	}
	else
	{
		ret = Datagram_GetMessage(chan, message);
	}

	// see if this connection has timed out
	if (ret == 0 && chan->remoteAddress.type == NA_IP)
	{
		if (net_time - chan->lastReceived / 1000.0 > net_messagetimeout->value)
		{
			NET_Close(chan);
			return -1;
		}
	}


	if (ret > 0)
	{
		if (chan->remoteAddress.type == NA_IP)
		{
			chan->lastReceived = net_time * 1000;
			if (ret == 1)
			{
				messagesReceived++;
			}
			else if (ret == 2)
			{
				unreliableMessagesReceived++;
			}
		}
	}

	return ret;
}

//	Try to send a complete length+message unit over the reliable stream.
// returns 0 if the message cannot be delivered reliably, but the connection
//        is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died
int NET_SendMessage(netchan_t* chan, QMsg* data)
{
	if (!chan)
	{
		return -1;
	}

	if (chan->disconnected)
	{
		common->Printf("NET_SendMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();
	int r;
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		r = Loop_SendMessage(chan, data);
	}
	else
	{
		r = Datagram_SendMessage(chan, data);
	}
	if (r == 1 && chan->remoteAddress.type == NA_IP)
	{
		messagesSent++;
	}

	return r;
}

int NET_SendUnreliableMessage(netchan_t* chan, QMsg* data)
{
	if (!chan)
	{
		return -1;
	}

	if (chan->disconnected)
	{
		common->Printf("NET_SendMessage: disconnected socket\n");
		return -1;
	}

	SetNetTime();
	int r;
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		r = Loop_SendUnreliableMessage(chan, data);
	}
	else
	{
		r = Datagram_SendUnreliableMessage(chan, data);
	}
	if (r == 1 && chan->remoteAddress.type == NA_IP)
	{
		unreliableMessagesSent++;
	}

	return r;
}

//	Returns true or false if the given qsocket can currently accept a
// message to be transmitted.
bool NET_CanSendMessage(netchan_t* chan)
{
	if (!chan)
	{
		return false;
	}

	if (chan->disconnected)
	{
		return false;
	}

	SetNetTime();

	int r;
	if (chan->remoteAddress.type == NA_LOOPBACK)
	{
		r = Loop_CanSendMessage(chan);
	}
	else
	{
		r = Datagram_CanSendMessage(chan);
	}

	return r;
}

static void NET_Listen_f()
{
	if (Cmd_Argc() != 2)
	{
		common->Printf("\"listen\" is \"%u\"\n", net_listening ? 1 : 0);
		return;
	}

	net_listening = String::Atoi(Cmd_Argv(1)) ? true : false;

	if (datagram_initialized)
	{
		Datagram_Listen(net_listening);
	}
}

static void NET_Port_f()
{
	int n;

	if (Cmd_Argc() != 2)
	{
		common->Printf("\"port\" is \"%u\"\n", net_hostport);
		return;
	}

	n = String::Atoi(Cmd_Argv(1));
	if (n < 1 || n > 65534)
	{
		common->Printf("Bad value, must be between 1 and 65534\n");
		return;
	}

	DEFAULTnet_hostport = n;
	net_hostport = n;

	if (net_listening)
	{
		// force a change to the new port
		Cbuf_AddText("listen 0\n");
		Cbuf_AddText("listen 1\n");
	}
}

void NETQH_Init()
{
	DEFAULTnet_hostport = GGameType & GAME_Hexen2 ? 26900 : 26000;

	int i = COM_CheckParm("-port");
	if (!i)
	{
		i = COM_CheckParm("-udpport");
	}

	if (i)
	{
		if (i < COM_Argc() - 1)
		{
			DEFAULTnet_hostport = String::Atoi(COM_Argv(i + 1));
		}
		else
		{
			common->FatalError("NET_Init: you must specify a number after -port");
		}
	}
	net_hostport = DEFAULTnet_hostport;

	if (COM_CheckParm("-listen") || com_dedicated->integer)
	{
		net_listening = true;
	}

	SetNetTime();

	net_messagetimeout = Cvar_Get("net_messagetimeout", "300", 0);
	sv_hostname = Cvar_Get("hostname", "UNNAMED", CVAR_ARCHIVE);

	Cmd_AddCommand("listen", NET_Listen_f);
	Cmd_AddCommand("port", NET_Port_f);

	// initialize all the drivers
	int controlSocket = Datagram_Init();
	if (controlSocket != -1)
	{
		datagram_initialized = true;
		if (net_listening)
		{
			Datagram_Listen(true);
		}
	}
}

void NETQH_Shutdown()
{
	if (datagram_initialized)
	{
		Datagram_Shutdown();
		datagram_initialized = false;
	}
}
