/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_dgrm.c

// This is enables a simple IP banning mechanism
#define BAN_TEST

#ifdef BAN_TEST
#if defined(_WIN32)
#include <windows.h>
#elif defined (NeXT) || defined __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
#else
#define AF_INET 		2	/* internet */
struct in_addr
{
	union
	{
		struct { unsigned char s_b1,s_b2,s_b3,s_b4; } S_un_b;
		struct { unsigned short s_w1,s_w2; } S_un_w;
		unsigned long S_addr;
	} S_un;
};
#define	s_addr	S_un.S_addr	/* can be used for most tcp & ip code */
struct sockaddr_in
{
    short			sin_family;
    unsigned short	sin_port;
	struct in_addr	sin_addr;
    char			sin_zero[8];
};
char *inet_ntoa(struct in_addr in);
unsigned long inet_addr(const char *cp);
#endif
#endif	// BAN_TEST

#include "quakedef.h"
#include "net_dgrm.h"
#include "net_udp.h"

/* statistic counters */
int	packetsSent = 0;
int	packetsReSent = 0;
int packetsReceived = 0;
int receivedDuplicateCount = 0;
int shortPacketCount = 0;
int droppedDatagrams;

static int			udp_controlSock;
static qboolean		udp_initialized;

static int myDriverLevel;

struct
{
	unsigned int	length;
	unsigned int	sequence;
	byte			data[MAX_DATAGRAM];
} packetBuffer;

extern enum {m_none, m_main, m_singleplayer, m_load, m_save, m_multiplayer, m_setup, m_net, m_options, m_video, m_keys, m_help, m_quit, m_lanconfig, m_gameoptions, m_search, m_slist} m_state, m_return_state;
extern qboolean m_return_onerror;
extern char m_return_reason[32];


#ifdef DEBUG
char *StrAddr (struct qsockaddr *addr)
{
	static char buf[34];
	byte *p = (byte *)addr;
	int n;

	for (n = 0; n < 16; n++)
		sprintf (buf + n * 2, "%02x", *p++);
	return buf;
}
#endif


#ifdef BAN_TEST
unsigned long banAddr = 0x00000000;
unsigned long banMask = 0xffffffff;

void NET_Ban_f (void)
{
	char	addrStr [32];
	char	maskStr [32];
	void	(*print) (const char *fmt, ...);

	if (cmd_source == src_command)
	{
		if (!sv.active)
		{
			Cmd_ForwardToServer ();
			return;
		}
		print = Con_Printf;
	}
	else
	{
		if (pr_global_struct->deathmatch && !host_client->privileged)
			return;
		print = SV_ClientPrintf;
	}

	switch (Cmd_Argc ())
	{
		case 1:
			if (((struct in_addr *)&banAddr)->s_addr)
			{
				QStr::Cpy(addrStr, inet_ntoa(*(struct in_addr *)&banAddr));
				QStr::Cpy(maskStr, inet_ntoa(*(struct in_addr *)&banMask));
				print("Banning %s [%s]\n", addrStr, maskStr);
			}
			else
				print("Banning not active\n");
			break;

		case 2:
			if (QStr::ICmp(Cmd_Argv(1), "off") == 0)
				banAddr = 0x00000000;
			else
				banAddr = inet_addr(Cmd_Argv(1));
			banMask = 0xffffffff;
			break;

		case 3:
			banAddr = inet_addr(Cmd_Argv(1));
			banMask = inet_addr(Cmd_Argv(2));
			break;

		default:
			print("BAN ip_address [mask]\n");
			break;
	}
}
#endif


int Datagram_SendMessage (qsocket_t *sock, QMsg *data)
{
	unsigned int	packetLen;
	unsigned int	dataLen;
	unsigned int	eom;

#ifdef DEBUG
	if (data->cursize == 0)
		Sys_Error("Datagram_SendMessage: zero length message\n");

	if (data->cursize > NET_MAXMESSAGE)
		Sys_Error("Datagram_SendMessage: message too big %u\n", data->cursize);

	if (sock->canSend == false)
		Sys_Error("SendMessage: called with canSend == false\n");
#endif

	Com_Memcpy(sock->sendMessage, data->_data, data->cursize);
	sock->sendMessageLength = data->cursize;

	if (data->cursize <= MAX_DATAGRAM)
	{
		dataLen = data->cursize;
		eom = NETFLAG_EOM;
	}
	else
	{
		dataLen = MAX_DATAGRAM;
		eom = 0;
	}
	packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(sock->sendSequence++);
	Com_Memcpy(packetBuffer.data, sock->sendMessage, dataLen);

	sock->canSend = false;

	if (UDP_Write(sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	sock->lastSendTime = net_time;
	packetsSent++;
	return 1;
}


int SendMessageNext (qsocket_t *sock)
{
	unsigned int	packetLen;
	unsigned int	dataLen;
	unsigned int	eom;

	if (sock->sendMessageLength <= MAX_DATAGRAM)
	{
		dataLen = sock->sendMessageLength;
		eom = NETFLAG_EOM;
	}
	else
	{
		dataLen = MAX_DATAGRAM;
		eom = 0;
	}
	packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(sock->sendSequence++);
	Com_Memcpy(packetBuffer.data, sock->sendMessage, dataLen);

	sock->sendNext = false;

	if (UDP_Write(sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	sock->lastSendTime = net_time;
	packetsSent++;
	return 1;
}


int ReSendMessage (qsocket_t *sock)
{
	unsigned int	packetLen;
	unsigned int	dataLen;
	unsigned int	eom;

	if (sock->sendMessageLength <= MAX_DATAGRAM)
	{
		dataLen = sock->sendMessageLength;
		eom = NETFLAG_EOM;
	}
	else
	{
		dataLen = MAX_DATAGRAM;
		eom = 0;
	}
	packetLen = NET_HEADERSIZE + dataLen;

	packetBuffer.length = BigLong(packetLen | (NETFLAG_DATA | eom));
	packetBuffer.sequence = BigLong(sock->sendSequence - 1);
	Com_Memcpy(packetBuffer.data, sock->sendMessage, dataLen);

	sock->sendNext = false;

	if (UDP_Write(sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	sock->lastSendTime = net_time;
	packetsReSent++;
	return 1;
}


qboolean Datagram_CanSendMessage (qsocket_t *sock)
{
	if (sock->sendNext)
		SendMessageNext (sock);

	return sock->canSend;
}


qboolean Datagram_CanSendUnreliableMessage (qsocket_t *sock)
{
	return true;
}


int Datagram_SendUnreliableMessage (qsocket_t *sock, QMsg *data)
{
	int 	packetLen;

#ifdef DEBUG
	if (data->cursize == 0)
		Sys_Error("Datagram_SendUnreliableMessage: zero length message\n");

	if (data->cursize > MAX_DATAGRAM)
		Sys_Error("Datagram_SendUnreliableMessage: message too big %u\n", data->cursize);
#endif

	packetLen = NET_HEADERSIZE + data->cursize;

	packetBuffer.length = BigLong(packetLen | NETFLAG_UNRELIABLE);
	packetBuffer.sequence = BigLong(sock->unreliableSendSequence++);
	Com_Memcpy(packetBuffer.data, data->_data, data->cursize);

	if (UDP_Write(sock->socket, (byte *)&packetBuffer, packetLen, &sock->addr) == -1)
		return -1;

	packetsSent++;
	return 1;
}


int	Datagram_GetMessage (qsocket_t *sock)
{
	unsigned int	length;
	unsigned int	flags;
	int				ret = 0;
	struct qsockaddr readaddr;
	unsigned int	sequence;
	unsigned int	count;

	if (!sock->canSend)
		if ((net_time - sock->lastSendTime) > 1.0)
			ReSendMessage (sock);

	while(1)
	{	
		length = UDP_Read(sock->socket, (byte *)&packetBuffer, NET_DATAGRAMSIZE, &readaddr);

//	if ((rand() & 255) > 220)
//		continue;

		if (length == 0)
			break;

		if (length == -1)
		{
			Con_Printf("Read error\n");
			return -1;
		}

		if (UDP_AddrCompare(&readaddr, &sock->addr) != 0)
		{
#ifdef DEBUG
			Con_DPrintf("Forged packet received\n");
			Con_DPrintf("Expected: %s\n", StrAddr (&sock->addr));
			Con_DPrintf("Received: %s\n", StrAddr (&readaddr));
#endif
			continue;
		}

		if (length < NET_HEADERSIZE)
		{
			shortPacketCount++;
			continue;
		}

		length = BigLong(packetBuffer.length);
		flags = length & (~NETFLAG_LENGTH_MASK);
		length &= NETFLAG_LENGTH_MASK;

		if (flags & NETFLAG_CTL)
			continue;

		sequence = BigLong(packetBuffer.sequence);
		packetsReceived++;

		if (flags & NETFLAG_UNRELIABLE)
		{
			if (sequence < sock->unreliableReceiveSequence)
			{
				Con_DPrintf("Got a stale datagram\n");
				ret = 0;
				break;
			}
			if (sequence != sock->unreliableReceiveSequence)
			{
				count = sequence - sock->unreliableReceiveSequence;
				droppedDatagrams += count;
				Con_DPrintf("Dropped %u datagram(s)\n", count);
			}
			sock->unreliableReceiveSequence = sequence + 1;

			length -= NET_HEADERSIZE;

			net_message.Clear();
			net_message.WriteData(packetBuffer.data, length);

			ret = 2;
			break;
		}

		if (flags & NETFLAG_ACK)
		{
			if (sequence != (sock->sendSequence - 1))
			{
				Con_DPrintf("Stale ACK received\n");
				continue;
			}
			if (sequence == sock->ackSequence)
			{
				sock->ackSequence++;
				if (sock->ackSequence != sock->sendSequence)
					Con_DPrintf("ack sequencing error\n");
			}
			else
			{
				Con_DPrintf("Duplicate ACK received\n");
				continue;
			}
			sock->sendMessageLength -= MAX_DATAGRAM;
			if (sock->sendMessageLength > 0)
			{
				Com_Memcpy(sock->sendMessage, sock->sendMessage+MAX_DATAGRAM, sock->sendMessageLength);
				sock->sendNext = true;
			}
			else
			{
				sock->sendMessageLength = 0;
				sock->canSend = true;
			}
			continue;
		}

		if (flags & NETFLAG_DATA)
		{
			packetBuffer.length = BigLong(NET_HEADERSIZE | NETFLAG_ACK);
			packetBuffer.sequence = BigLong(sequence);
			UDP_Write(sock->socket, (byte *)&packetBuffer, NET_HEADERSIZE, &readaddr);

			if (sequence != sock->receiveSequence)
			{
				receivedDuplicateCount++;
				continue;
			}
			sock->receiveSequence++;

			length -= NET_HEADERSIZE;

			if (flags & NETFLAG_EOM)
			{
				net_message.Clear();
				net_message.WriteData(sock->receiveMessage, sock->receiveMessageLength);
				net_message.WriteData(packetBuffer.data, length);
				sock->receiveMessageLength = 0;

				ret = 1;
				break;
			}

			Com_Memcpy(sock->receiveMessage + sock->receiveMessageLength, packetBuffer.data, length);
			sock->receiveMessageLength += length;
			continue;
		}
	}

	if (sock->sendNext)
		SendMessageNext (sock);

	return ret;
}


void PrintStats(qsocket_t *s)
{
	Con_Printf("canSend = %4u   \n", s->canSend);
	Con_Printf("sendSeq = %4u   ", s->sendSequence);
	Con_Printf("recvSeq = %4u   \n", s->receiveSequence);
	Con_Printf("\n");
}

void NET_Stats_f (void)
{
	qsocket_t	*s;

	if (Cmd_Argc () == 1)
	{
		Con_Printf("unreliable messages sent   = %i\n", unreliableMessagesSent);
		Con_Printf("unreliable messages recv   = %i\n", unreliableMessagesReceived);
		Con_Printf("reliable messages sent     = %i\n", messagesSent);
		Con_Printf("reliable messages received = %i\n", messagesReceived);
		Con_Printf("packetsSent                = %i\n", packetsSent);
		Con_Printf("packetsReSent              = %i\n", packetsReSent);
		Con_Printf("packetsReceived            = %i\n", packetsReceived);
		Con_Printf("receivedDuplicateCount     = %i\n", receivedDuplicateCount);
		Con_Printf("shortPacketCount           = %i\n", shortPacketCount);
		Con_Printf("droppedDatagrams           = %i\n", droppedDatagrams);
	}
	else if (QStr::Cmp(Cmd_Argv(1), "*") == 0)
	{
		for (s = net_activeSockets; s; s = s->next)
			PrintStats(s);
		for (s = net_freeSockets; s; s = s->next)
			PrintStats(s);
	}
	else
	{
		for (s = net_activeSockets; s; s = s->next)
			if (QStr::ICmp(Cmd_Argv(1), s->address) == 0)
				break;
		if (s == NULL)
			for (s = net_freeSockets; s; s = s->next)
				if (QStr::ICmp(Cmd_Argv(1), s->address) == 0)
					break;
		if (s == NULL)
			return;
		PrintStats(s);
	}
}


static qboolean testInProgress = false;
static int		testPollCount;
static int		testSocket;

static void Test_Poll(void);
PollProcedure	testPollProcedure = {NULL, 0.0, Test_Poll};

static void Test_Poll(void)
{
	struct qsockaddr clientaddr;
	int		control;
	int		len;
	char	name[32];
	char	address[64];
	int		colors;
	int		frags;
	int		connectTime;
	byte	playerNumber;

	while (1)
	{
		len = UDP_Read(testSocket, net_message._data, net_message.maxsize, &clientaddr);
		if (len < sizeof(int))
			break;

		net_message.cursize = len;

		net_message.BeginReadingOOB();
		control = BigLong(*((int *)net_message._data));
		net_message.ReadLong();
		if (control == -1)
			break;
		if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
			break;
		if ((control & NETFLAG_LENGTH_MASK) != len)
			break;

		if (net_message.ReadByte() != CCREP_PLAYER_INFO)
			Sys_Error("Unexpected repsonse to Player Info request\n");

		playerNumber = net_message.ReadByte();
		QStr::Cpy(name, net_message.ReadString2());
		colors = net_message.ReadLong();
		frags = net_message.ReadLong();
		connectTime = net_message.ReadLong();
		QStr::Cpy(address, net_message.ReadString2());

		Con_Printf("%s\n  frags:%3i  colors:%u %u  time:%u\n  %s\n", name, frags, colors >> 4, colors & 0x0f, connectTime / 60, address);
	}

	testPollCount--;
	if (testPollCount)
	{
		SchedulePollProcedure(&testPollProcedure, 0.1);
	}
	else
	{
		UDP_OpenSocket(testSocket);
		testInProgress = false;
	}
}

static void Test_f (void)
{
	char	*host;
	int		n;
	int		max = MAX_SCOREBOARD;
	netadr_t sendaddr;
	struct qsockaddr sendaddr_old;

	if (testInProgress)
		return;

	host = Cmd_Argv (1);

	if (host && hostCacheCount)
	{
		for (n = 0; n < hostCacheCount; n++)
			if (QStr::ICmp(host, hostcache[n].name) == 0)
			{
				if (hostcache[n].driver != myDriverLevel)
					continue;
				max = hostcache[n].maxusers;
				Com_Memcpy(&sendaddr_old, &hostcache[n].addr, sizeof(struct qsockaddr));
				break;
			}
		if (n < hostCacheCount)
			goto JustDoIt;
	}

	if (!udp_initialized)
		return;

	// see if we can resolve the host name
	if (UDP_GetAddrFromName(host, &sendaddr) == -1)
		return;
	NetadrToSockadr(&sendaddr, (struct sockaddr_in*)&sendaddr_old);

JustDoIt:
	testSocket = UDP_OpenSocket(0);
	if (testSocket == -1)
		return;

	testInProgress = true;
	testPollCount = 20;

	for (n = 0; n < max; n++)
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREQ_PLAYER_INFO);
		net_message.WriteByte(n);
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(testSocket, net_message._data, net_message.cursize, &sendaddr_old);
	}
	net_message.Clear();
	SchedulePollProcedure(&testPollProcedure, 0.1);
}


static qboolean test2InProgress = false;
static int		test2Socket;

static void Test2_Poll(void);
PollProcedure	test2PollProcedure = {NULL, 0.0, Test2_Poll};

static void Test2_Poll(void)
{
	struct qsockaddr clientaddr;
	int		control;
	int		len;
	char	name[256];
	char	value[256];

	name[0] = 0;

	len = UDP_Read(test2Socket, net_message._data, net_message.maxsize, &clientaddr);
	if (len < sizeof(int))
		goto Reschedule;

	net_message.cursize = len;

	net_message.BeginReadingOOB();
	control = BigLong(*((int *)net_message._data));
	net_message.ReadLong();
	if (control == -1)
		goto Error;
	if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
		goto Error;
	if ((control & NETFLAG_LENGTH_MASK) != len)
		goto Error;

	if (net_message.ReadByte() != CCREP_RULE_INFO)
		goto Error;

	QStr::Cpy(name, net_message.ReadString2());
	if (name[0] == 0)
		goto Done;
	QStr::Cpy(value, net_message.ReadString2());

	Con_Printf("%-16.16s  %-16.16s\n", name, value);

	net_message.Clear();
	// save space for the header, filled in later
	net_message.WriteLong(0);
	net_message.WriteByte(CCREQ_RULE_INFO);
	net_message.WriteString2(name);
	*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	UDP_Write(test2Socket, net_message._data, net_message.cursize, &clientaddr);
	net_message.Clear();

Reschedule:
	SchedulePollProcedure(&test2PollProcedure, 0.05);
	return;

Error:
	Con_Printf("Unexpected repsonse to Rule Info request\n");
Done:
	UDP_OpenSocket(test2Socket);
	test2InProgress = false;
	return;
}

static void Test2_f (void)
{
	char	*host;
	int		n;
	netadr_t sendaddr;
	struct qsockaddr sendaddr_old;

	if (test2InProgress)
		return;

	host = Cmd_Argv (1);

	if (host && hostCacheCount)
	{
		for (n = 0; n < hostCacheCount; n++)
			if (QStr::ICmp(host, hostcache[n].name) == 0)
			{
				if (hostcache[n].driver != myDriverLevel)
					continue;
				Com_Memcpy(&sendaddr_old, &hostcache[n].addr, sizeof(struct qsockaddr));
				break;
			}
		if (n < hostCacheCount)
			goto JustDoIt;
	}

	if (!udp_initialized)
		return;

	// see if we can resolve the host name
	if (UDP_GetAddrFromName(host, &sendaddr) == -1)
		return;
	NetadrToSockadr(&sendaddr, (struct sockaddr_in*)&sendaddr_old);

JustDoIt:
	test2Socket = UDP_OpenSocket(0);
	if (test2Socket == -1)
		return;

	test2InProgress = true;

	net_message.Clear();
	// save space for the header, filled in later
	net_message.WriteLong(0);
	net_message.WriteByte(CCREQ_RULE_INFO);
	net_message.WriteString2("");
	*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	UDP_Write(test2Socket, net_message._data, net_message.cursize, &sendaddr_old);
	net_message.Clear();
	SchedulePollProcedure(&test2PollProcedure, 0.05);
}


int Datagram_Init (void)
{
	myDriverLevel = net_driverlevel;
	Cmd_AddCommand ("net_stats", NET_Stats_f);

	if (COM_CheckParm("-nolan"))
		return -1;

	int csock = UDP_Init();
	if (csock != -1)
	{
		udp_initialized = true;
		udp_controlSock = csock;
	}

#ifdef BAN_TEST
	Cmd_AddCommand ("ban", NET_Ban_f);
#endif
	Cmd_AddCommand ("test", Test_f);
	Cmd_AddCommand ("test2", Test2_f);

	return 0;
}


void Datagram_Shutdown (void)
{
//
// shutdown the lan drivers
//
	if (udp_initialized)
	{
		UDP_Shutdown();
		udp_initialized = false;
	}
}


void Datagram_Close (qsocket_t *sock)
{
	UDP_OpenSocket(sock->socket);
}


void Datagram_Listen (qboolean state)
{
	if (udp_initialized)
		UDP_Listen(state);
}


static qsocket_t *_Datagram_CheckNewConnections (void)
{
	netadr_t clientaddr;
	struct qsockaddr clientaddr_old;
	netadr_t newaddr;
	struct qsockaddr newaddr_old;
	int			newsock;
	int			acceptsock;
	qsocket_t	*sock;
	qsocket_t	*s;
	int			len;
	int			command;
	int			control;
	int			ret;

	acceptsock = UDP_CheckNewConnections();
	if (acceptsock == -1)
		return NULL;

	net_message.Clear();

	len = UDP_Read(acceptsock, net_message._data, net_message.maxsize, &clientaddr_old);
	SockadrToNetadr((struct sockaddr_in*)&clientaddr_old, &clientaddr);
	if (len < sizeof(int))
		return NULL;
	net_message.cursize = len;

	net_message.BeginReadingOOB();
	control = BigLong(*((int *)net_message._data));
	net_message.ReadLong();
	if (control == -1)
		return NULL;
	if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
		return NULL;
	if ((control & NETFLAG_LENGTH_MASK) != len)
		return NULL;

	command = net_message.ReadByte();
	if (command == CCREQ_SERVER_INFO)
	{
		if (QStr::Cmp(net_message.ReadString2(), "QUAKE") != 0)
			return NULL;

		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_SERVER_INFO);
		UDP_GetSocketAddr(acceptsock, &newaddr);
		net_message.WriteString2(UDP_AddrToString(&newaddr));
		net_message.WriteString2(hostname->string);
		net_message.WriteString2(sv.name);
		net_message.WriteByte(net_activeconnections);
		net_message.WriteByte(svs.maxclients);
		net_message.WriteByte(NET_PROTOCOL_VERSION);
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
		net_message.Clear();
		return NULL;
	}

	if (command == CCREQ_PLAYER_INFO)
	{
		int			playerNumber;
		int			activeNumber;
		int			clientNumber;
		client_t	*client;
		
		playerNumber = net_message.ReadByte();
		activeNumber = -1;
		for (clientNumber = 0, client = svs.clients; clientNumber < svs.maxclients; clientNumber++, client++)
		{
			if (client->active)
			{
				activeNumber++;
				if (activeNumber == playerNumber)
					break;
			}
		}
		if (clientNumber == svs.maxclients)
			return NULL;

		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_PLAYER_INFO);
		net_message.WriteByte(playerNumber);
		net_message.WriteString2(client->name);
		net_message.WriteLong(client->colors);
		net_message.WriteLong((int)client->edict->v.frags);
		net_message.WriteLong((int)(net_time - client->netconnection->connecttime));
		net_message.WriteString2(client->netconnection->address);
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
		net_message.Clear();

		return NULL;
	}

	if (command == CCREQ_RULE_INFO)
	{
		const char	*prevCvarName;
		QCvar*		var;

		// find the search start location
		prevCvarName = net_message.ReadString2();
		if (*prevCvarName)
		{
			var = Cvar_FindVar (prevCvarName);
			if (!var)
				return NULL;
			var = var->next;
		}
		else
			var = cvar_vars;

		// search for the next server cvar
		while (var)
		{
			if (var->flags & CVAR_SERVERINFO)
				break;
			var = var->next;
		}

		// send the response

		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_RULE_INFO);
		if (var)
		{
			net_message.WriteString2(var->name);
			net_message.WriteString2(var->string);
		}
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
		net_message.Clear();

		return NULL;
	}

	if (command != CCREQ_CONNECT)
		return NULL;

	if (QStr::Cmp(net_message.ReadString2(), "QUAKE") != 0)
		return NULL;

	if (net_message.ReadByte() != NET_PROTOCOL_VERSION)
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_REJECT);
		net_message.WriteString2("Incompatible version.\n");
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
		net_message.Clear();
		return NULL;
	}

#ifdef BAN_TEST
	// check for a ban
	if (clientaddr_old.sa_family == AF_INET)
	{
		unsigned long testAddr;
		testAddr = ((struct sockaddr_in *)&clientaddr_old)->sin_addr.s_addr;
		if ((testAddr & banMask) == banAddr)
		{
			net_message.Clear();
			// save space for the header, filled in later
			net_message.WriteLong(0);
			net_message.WriteByte(CCREP_REJECT);
			net_message.WriteString2("You have been banned.\n");
			*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
			UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
			net_message.Clear();
			return NULL;
		}
	}
#endif

	// see if this guy is already connected
	for (s = net_activeSockets; s; s = s->next)
	{
		if (s->driver != net_driverlevel)
			continue;
		ret = UDP_AddrCompare(&clientaddr_old, &s->addr);
		if (ret >= 0)
		{
			// is this a duplicate connection reqeust?
			if (ret == 0 && net_time - s->connecttime < 2.0)
			{
				// yes, so send a duplicate reply
				net_message.Clear();
				// save space for the header, filled in later
				net_message.WriteLong(0);
				net_message.WriteByte(CCREP_ACCEPT);
				UDP_GetSocketAddr(s->socket, &newaddr);
				NetadrToSockadr(&newaddr, (struct sockaddr_in*)&newaddr_old);
				net_message.WriteLong(UDP_GetSocketPort(&newaddr_old));
				*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
				UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
				net_message.Clear();
				return NULL;
			}
			// it's somebody coming back in from a crash/disconnect
			// so close the old qsocket and let their retry get them back in
			NET_Close(s);
			return NULL;
		}
	}

	// allocate a QSocket
	sock = NET_NewQSocket ();
	if (sock == NULL)
	{
		// no room; try to let him know
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_REJECT);
		net_message.WriteString2("Server is full.\n");
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
		net_message.Clear();
		return NULL;
	}

	// allocate a network socket
	newsock = UDP_OpenSocket(0);
	if (newsock == -1)
	{
		NET_FreeQSocket(sock);
		return NULL;
	}

	// connect to the client
	if (UDP_Connect(newsock, &clientaddr_old) == -1)
	{
		UDP_OpenSocket(newsock);
		NET_FreeQSocket(sock);
		return NULL;
	}

	// everything is allocated, just fill in the details	
	sock->socket = newsock;
	sock->addr = clientaddr_old;
	QStr::Cpy(sock->address, UDP_AddrToString(&clientaddr));

	// send him back the info about the server connection he has been allocated
	net_message.Clear();
	// save space for the header, filled in later
	net_message.WriteLong(0);
	net_message.WriteByte(CCREP_ACCEPT);
	UDP_GetSocketAddr(newsock, &newaddr);
	NetadrToSockadr(&newaddr, (struct sockaddr_in*)&newaddr_old);
	net_message.WriteLong(UDP_GetSocketPort(&newaddr_old));
//	net_message.WriteString2(dfunc.AddrToString(&newaddr));
	*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr_old);
	net_message.Clear();

	return sock;
}

qsocket_t *Datagram_CheckNewConnections (void)
{
	qsocket_t *ret = NULL;

	if (udp_initialized)
		ret = _Datagram_CheckNewConnections();
	return ret;
}


static void _Datagram_SearchForHosts (qboolean xmit)
{
	int		ret;
	int		n;
	int		i;
	netadr_t readaddr;
	struct qsockaddr readaddr_old;
	netadr_t	myaddr;
	struct qsockaddr myaddr_old;
	int		control;

	UDP_GetSocketAddr(udp_controlSock, &myaddr);
	NetadrToSockadr(&myaddr, (struct sockaddr_in*)&myaddr_old);
	if (xmit)
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREQ_SERVER_INFO);
		net_message.WriteString2("QUAKE");
		net_message.WriteByte(NET_PROTOCOL_VERSION);
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Broadcast(udp_controlSock, net_message._data, net_message.cursize);
		net_message.Clear();
	}

	while ((ret = UDP_Read(udp_controlSock, net_message._data, net_message.maxsize, &readaddr_old)) > 0)
	{
		if (ret < sizeof(int))
			continue;
		net_message.cursize = ret;

		// don't answer our own query
		if (UDP_AddrCompare(&readaddr_old, &myaddr_old) >= 0)
			continue;

		// is the cache full?
		if (hostCacheCount == HOSTCACHESIZE)
			continue;

		net_message.BeginReadingOOB();
		control = BigLong(*((int *)net_message._data));
		net_message.ReadLong();
		if (control == -1)
			continue;
		if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
			continue;
		if ((control & NETFLAG_LENGTH_MASK) != ret)
			continue;

		if (net_message.ReadByte() != CCREP_SERVER_INFO)
			continue;

		UDP_GetAddrFromName(net_message.ReadString2(), &readaddr);
		NetadrToSockadr(&readaddr, (struct sockaddr_in*)&readaddr_old);
		// search the cache for this server
		for (n = 0; n < hostCacheCount; n++)
			if (UDP_AddrCompare(&readaddr_old, &hostcache[n].addr) == 0)
				break;

		// is it already there?
		if (n < hostCacheCount)
			continue;

		// add it
		hostCacheCount++;
		QStr::Cpy(hostcache[n].name, net_message.ReadString2());
		QStr::Cpy(hostcache[n].map, net_message.ReadString2());
		hostcache[n].users = net_message.ReadByte();
		hostcache[n].maxusers = net_message.ReadByte();
		if (net_message.ReadByte() != NET_PROTOCOL_VERSION)
		{
			QStr::Cpy(hostcache[n].cname, hostcache[n].name);
			hostcache[n].cname[14] = 0;
			QStr::Cpy(hostcache[n].name, "*");
			QStr::Cat(hostcache[n].name, sizeof(hostcache[n].name), hostcache[n].cname);
		}
		Com_Memcpy(&hostcache[n].addr, &readaddr_old, sizeof(struct qsockaddr));
		hostcache[n].driver = net_driverlevel;
		QStr::Cpy(hostcache[n].cname, UDP_AddrToString(&readaddr));

		// check for a name conflict
		for (i = 0; i < hostCacheCount; i++)
		{
			if (i == n)
				continue;
			if (QStr::ICmp(hostcache[n].name, hostcache[i].name) == 0)
			{
				i = QStr::Length(hostcache[n].name);
				if (i < 15 && hostcache[n].name[i-1] > '8')
				{
					hostcache[n].name[i] = '0';
					hostcache[n].name[i+1] = 0;
				}
				else
					hostcache[n].name[i-1]++;
				i = -1;
			}
		}
	}
}

void Datagram_SearchForHosts (qboolean xmit)
{
	if (udp_initialized)
		_Datagram_SearchForHosts (xmit);
}


static qsocket_t *_Datagram_Connect (char *host)
{
	netadr_t sendaddr;
	struct qsockaddr sendaddr_old;
	struct qsockaddr readaddr;
	qsocket_t	*sock;
	int			newsock;
	int			ret;
	int			reps;
	double		start_time;
	int			control;
	const char	*reason;

	// see if we can resolve the host name
	if (UDP_GetAddrFromName(host, &sendaddr) == -1)
		return NULL;
	NetadrToSockadr(&sendaddr, (struct sockaddr_in*)&sendaddr_old);

	newsock = UDP_OpenSocket(0);
	if (newsock == -1)
		return NULL;

	sock = NET_NewQSocket ();
	if (sock == NULL)
		goto ErrorReturn2;
	sock->socket = newsock;

	// connect to the host
	if (UDP_Connect(newsock, &sendaddr_old) == -1)
		goto ErrorReturn;

	// send the connection request
	Con_Printf("trying...\n"); SCR_UpdateScreen ();
	start_time = net_time;

	for (reps = 0; reps < 3; reps++)
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREQ_CONNECT);
		net_message.WriteString2("QUAKE");
		net_message.WriteByte(NET_PROTOCOL_VERSION);
		*((int *)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(newsock, net_message._data, net_message.cursize, &sendaddr_old);
		net_message.Clear();
		do
		{
			ret = UDP_Read(newsock, net_message._data, net_message.maxsize, &readaddr);
			// if we got something, validate it
			if (ret > 0)
			{
				// is it from the right place?
				if (UDP_AddrCompare(&readaddr, &sendaddr_old) != 0)
				{
#ifdef DEBUG
					Con_Printf("wrong reply address\n");
					Con_Printf("Expected: %s\n", StrAddr (&sendaddr_old));
					Con_Printf("Received: %s\n", StrAddr (&readaddr));
					SCR_UpdateScreen ();
#endif
					ret = 0;
					continue;
				}

				if (ret < sizeof(int))
				{
					ret = 0;
					continue;
				}

				net_message.cursize = ret;
				net_message.BeginReadingOOB();

				control = BigLong(*((int *)net_message._data));
				net_message.ReadLong();
				if (control == -1)
				{
					ret = 0;
					continue;
				}
				if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
				{
					ret = 0;
					continue;
				}
				if ((control & NETFLAG_LENGTH_MASK) != ret)
				{
					ret = 0;
					continue;
				}
			}
		}
		while (ret == 0 && (SetNetTime() - start_time) < 2.5);
		if (ret)
			break;
		Con_Printf("still trying...\n"); SCR_UpdateScreen ();
		start_time = SetNetTime();
	}

	if (ret == 0)
	{
		reason = "No Response";
		Con_Printf("%s\n", reason);
		QStr::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	if (ret == -1)
	{
		reason = "Network Error";
		Con_Printf("%s\n", reason);
		QStr::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	ret = net_message.ReadByte();
	if (ret == CCREP_REJECT)
	{
		reason = net_message.ReadString2();
		Con_Printf(reason);
		QStr::NCpy(m_return_reason, reason, 31);
		goto ErrorReturn;
	}

	if (ret == CCREP_ACCEPT)
	{
		Com_Memcpy(&sock->addr, &sendaddr_old, sizeof(struct qsockaddr));
		UDP_SetSocketPort(&sock->addr, net_message.ReadLong());
	}
	else
	{
		reason = "Bad Response";
		Con_Printf("%s\n", reason);
		QStr::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	UDP_GetNameFromAddr(&sendaddr, sock->address);

	Con_Printf ("Connection accepted\n");
	sock->lastMessageTime = SetNetTime();

	// switch the connection to the specified address
	if (UDP_Connect(newsock, &sock->addr) == -1)
	{
		reason = "Connect to Game failed";
		Con_Printf("%s\n", reason);
		QStr::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	m_return_onerror = false;
	return sock;

ErrorReturn:
	NET_FreeQSocket(sock);
ErrorReturn2:
	UDP_OpenSocket(newsock);
	if (m_return_onerror)
	{
		in_keyCatchers |= KEYCATCH_UI;
		m_state = m_return_state;
		m_return_onerror = false;
	}
	return NULL;
}

qsocket_t *Datagram_Connect (char *host)
{
	qsocket_t *ret = NULL;

	if (udp_initialized)
		ret = _Datagram_Connect (host);
	return ret;
}

//=============================================================================

int net_acceptsocket = -1;		// socket for fielding new connections
int net_controlsocket;
const char* net_interface;

//=============================================================================

void UDP_Shutdown (void)
{
	UDP_Listen (false);
	UDP_CloseSocket (net_controlsocket);
	SOCK_Shutdown();
}

//=============================================================================

void UDP_Listen (qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = UDP_OpenSocket (net_hostport)) == -1)
			Sys_Error ("UDP_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	UDP_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int UDP_OpenSocket (int port)
{
	int newsocket = SOCK_Open(net_interface, port);
	if (newsocket == 0)
		return -1;
	return newsocket;
}

//=============================================================================

int UDP_CloseSocket (int socket)
{
	SOCK_Close(socket);
	return 0;
}

//=============================================================================

int UDP_Connect (int socket, struct qsockaddr *addr)
{
	return 0;
}

//=============================================================================

int UDP_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	netadr_t From;
	int ret = SOCK_Recv(socket, buf, len, &From);
	if (ret == SOCKRECV_NO_DATA)
	{
		return 0;
	}
	if (ret == SOCKRECV_ERROR)
	{
		return -1;
	}
	NetadrToSockadr(&From, (struct sockaddr_in*)addr);
	return ret;
}

//=============================================================================

int UDP_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	netadr_t to;
	SockadrToNetadr((struct sockaddr_in*)addr, &to);
	int ret = SOCL_Send(socket, buf, len, &to);
	if (ret == SOCKSEND_WOULDBLOCK)
		return 0;
	return ret;
}

//=============================================================================

int UDP_Broadcast (int socket, byte *buf, int len)
{
	netadr_t to;
	Com_Memset(&to, 0, sizeof(to));
	to.type = NA_BROADCAST;
	to.port = BigShort(net_hostport);
	int ret = SOCL_Send(socket, buf, len, &to);
	if (ret == SOCKSEND_WOULDBLOCK)
		return 0;
	return ret;
}

//=============================================================================

char* UDP_AddrToString(netadr_t* addr)
{
	static char buffer[22];

	sprintf(buffer, "%d.%d.%d.%d:%d", addr->ip[0], addr->ip[1], addr->ip[2], addr->ip[3], BigShort(addr->port));
	return buffer;
}

//=============================================================================

int UDP_AddrCompare (struct qsockaddr *addr1_old, struct qsockaddr *addr2_old)
{
	netadr_t addr1;
	netadr_t addr2;
	SockadrToNetadr((struct sockaddr_in*)addr1_old, &addr1);
	SockadrToNetadr((struct sockaddr_in*)addr2_old, &addr2);
	if (addr1.type != addr2.type)
		return -1;

	if (addr1.ip[0] != addr2.ip[0] || addr1.ip[1] != addr2.ip[1] || addr1.ip[2] != addr2.ip[2] || addr1.ip[3] != addr2.ip[3])
		return -1;

	if (addr1.port != addr2.port)
		return 1;

	return 0;
}

//=============================================================================

int UDP_GetSocketPort (struct qsockaddr *addr_old)
{
	netadr_t addr;
	SockadrToNetadr((struct sockaddr_in*)addr_old, &addr);
	return BigShort(addr.port);
}


int UDP_SetSocketPort (struct qsockaddr *addr_old, int port)
{
	netadr_t addr;
	SockadrToNetadr((struct sockaddr_in*)addr_old, &addr);
	addr.port = BigShort(port);
	NetadrToSockadr(&addr, (struct sockaddr_in*)addr_old);
	return 0;
}

//=============================================================================
