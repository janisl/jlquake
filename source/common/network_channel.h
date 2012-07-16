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

#define MAX_MSGLEN_Q1       8192		// max length of a reliable message
#define MAX_MSGLEN_QW       1450		// max length of a reliable message
#define MAX_MSGLEN_H2       16384
#define MAX_MSGLEN_HW       7500		// max length of a reliable message
#define MAX_MSGLEN_Q2       1400		// max length of a message
#define MAX_MSGLEN_Q3       16384		// max length of a message, which may
										// be fragmented into multiple packets
#define MAX_MSGLEN_WOLF     32768		// increased for larger submodel entity counts
#define MAX_MSGLEN          32768		// biggest value of all of the above

#define MAX_DATAGRAM_QH     1024		// max length of unreliable message
#define MAX_DATAGRAM_QW     1450		// max length of unreliable message
#define MAX_DATAGRAM_HW     1400		// max length of unreliable message
#define MAX_DATAGRAM        1450		// biggest value of all of the above

#define MAX_PACKETLEN       1400		// max size of a network packet

enum netsrc_t
{
	NS_CLIENT,
	NS_SERVER
};

struct netchan_t
{
	netsrc_t sock;

	int dropped;			// between last packet and previous

	netadr_t remoteAddress;
	int qport;				// qport value to write when transmitting

	// sequencing variables
	int incomingSequence;
	int incomingReliableSequence;		// single bit, maintained local
	int outgoingSequence;
	int outgoingReliableSequence;		// single bit
	int lastReliableSequence;		// sequence number of last send
	int incomingAcknowledged;
	int incomingReliableAcknowledged;	// single bit

	// incoming fragment assembly buffer
	int fragmentSequence;
	int fragmentLength;
	byte fragmentBuffer[MAX_MSGLEN];

	//	Reliable message buffer for pre-Quake 3 games, message is copied to
	// this buffer when it is first transfered.
	//	For Quake 3 it's outgoing fragment buffer.
	// we need to space out the sending of large fragmented messages
	bool unsentFragments;
	int unsentFragmentStart;
	int reliableOrUnsentLength;
	byte reliableOrUnsentBuffer[MAX_MSGLEN];

	int lastReceived;		// for timeouts
	int lastSent;			// for retransmits

	// reliable staging and holding areas
	QMsg message;		// writing buffer to send to server
	byte messageBuffer[MAX_MSGLEN];

	// the statistics are cleared at each client begin, because
	// the server connecting process gives a bogus picture of the data
	float frameRate;
	int dropCount;			// dropped packets, cleared each level
	// bandwidth estimator
	double clearTime;			// if realtime > nc->cleartime, free to go
	double rate;				// seconds / byte
};

#define NET_NAMELEN_Q1          64

struct qsocket_t
{
	qsocket_t* next;
	double connecttime;

	qboolean disconnected;
	qboolean canSend;
	qboolean sendNext;

	int socket;

	char address[NET_NAMELEN_Q1];
};

// there needs to be enough loopback messages to hold a complete
// gamestate of maximum size
#define MAX_LOOPBACK    16

struct loopmsg_t
{
	byte data[MAX_MSGLEN];
	int datalen;
	byte type;
};

struct loopback_t
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int get;
	int send;
	bool canSend;
};

extern Cvar* sv_hostname;
extern loopback_t loopbacks[2];

int NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
void NET_SendLoopPacket(netsrc_t sock, int length, const void* data, int type);

#define NET_NAME_ID         "HEXENII"

#define Q1NET_PROTOCOL_VERSION    3
#define H2NET_PROTOCOL_VERSION    5

#define NET_HEADERSIZE      (2 * sizeof(unsigned int))
#define NET_DATAGRAMSIZE    (MAX_DATAGRAM_QH + NET_HEADERSIZE)

// NetHeader flags
#define NETFLAG_LENGTH_MASK 0x0000ffff
#define NETFLAG_DATA        0x00010000
#define NETFLAG_ACK         0x00020000
#define NETFLAG_NAK         0x00040000
#define NETFLAG_EOM         0x00080000
#define NETFLAG_UNRELIABLE  0x00100000
#define NETFLAG_CTL         0x80000000

#define CCREQ_CONNECT       0x01
#define CCREQ_SERVER_INFO   0x02
#define CCREQ_PLAYER_INFO   0x03
#define CCREQ_RULE_INFO     0x04

#define CCREP_ACCEPT        0x81
#define CCREP_REJECT        0x82
#define CCREP_SERVER_INFO   0x83
#define CCREP_PLAYER_INFO   0x84
#define CCREP_RULE_INFO     0x85

#define HOSTCACHESIZE   8

struct hostcache_t
{
	char name[16];
	char map[16];
	char cname[32];
	int users;
	int maxusers;
	int driver;
	netadr_t addr;
};

struct packetBuffer_t
{
	unsigned int length;
	unsigned int sequence;
	byte data[MAX_DATAGRAM_QH];
};

extern bool localconnectpending;
extern qsocket_t* loop_client;
extern qsocket_t* loop_server;
extern int hostCacheCount;
extern hostcache_t hostcache[HOSTCACHESIZE];
extern int net_activeconnections;
extern qsocket_t* net_activeSockets;
extern qsocket_t* net_freeSockets;
extern int net_numsockets;
extern double net_time;
extern int net_controlsocket;
extern bool tcpipAvailable;
extern int net_acceptsocket;
extern int net_hostport;
extern packetBuffer_t packetBuffer;
extern int packetsSent;
extern int packetsReSent;
extern int packetsReceived;
extern int receivedDuplicateCount;
extern int shortPacketCount;
extern int droppedDatagrams;
extern netadr_t banAddr;
extern netadr_t banMask;
extern int messagesSent;
extern int messagesReceived;
extern int unreliableMessagesSent;
extern int unreliableMessagesReceived;
extern int udp_controlSock;
extern bool udp_initialized;

qsocket_t* NET_NewQSocket();
void NET_FreeQSocket(qsocket_t*);
double SetNetTime();
void Loop_SearchForHosts(bool xmit);
qsocket_t* Loop_Connect(const char* host, netchan_t* chan);
qsocket_t* Loop_CheckNewConnections(netadr_t* outaddr);
int Loop_GetMessage(netchan_t* chan, QMsg* message);
int Loop_SendMessage(netchan_t* chan, QMsg* data);
int Loop_SendUnreliableMessage(netchan_t* chan, QMsg* data);
bool Loop_CanSendMessage(netchan_t* chan);
void Loop_Close(qsocket_t* sock, netchan_t* chan);
int  UDP_Init();
int  UDPNQ_OpenSocket(int port);
void UDP_Shutdown();
void UDP_Listen(bool state);
int  UDP_CloseSocket(int socket);
int  UDP_Read(int socket, byte* buf, int len, netadr_t* addr);
int  UDP_Write(int socket, byte* buf, int len, netadr_t* addr);
int  UDP_Broadcast(int socket, byte* buf, int len);
int  UDP_GetNameFromAddr(netadr_t* addr, char* name);
int  UDP_GetAddrFromName(const char* name, netadr_t* addr);
int  UDP_AddrCompare(netadr_t* addr1, netadr_t* addr2);
int Datagram_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
int SendMessageNext(qsocket_t* sock, netchan_t* chan);
int ReSendMessage(qsocket_t* sock, netchan_t* chan);
bool Datagram_CanSendMessage(qsocket_t* sock, netchan_t* chan);
int Datagram_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
int Datagram_GetMessage(qsocket_t* sock, netchan_t* chan, QMsg* message);
void NET_Ban_f();
void NET_Stats_f();
int Datagram_Init();
void Datagram_Shutdown();
void Datagram_Listen(bool state);
void Datagram_Close(qsocket_t* sock, netchan_t* chan);
qsocket_t* Datagram_CheckNewConnections(netadr_t* outaddr);
void Datagram_SearchForHosts(bool xmit);
qsocket_t* Datagram_Connect(const char* host, netchan_t* chan);
void NET_Close(qsocket_t* sock, netchan_t* chan);
// if a dead connection is returned by a get or send function, this function
// should be called when it is convenient
// Server calls when a client is kicked off for a game related misbehavior
// like an illegal protocal conversation.  Client calls when disconnecting
// from a server.
// A netcon_t number will not be reused until this function is called for it
