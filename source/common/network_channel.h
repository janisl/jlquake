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

	int dropCount;			// dropped packets, cleared each level
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

#define PACKET_HEADER   8

#define FRAGMENT_BIT    (1 << 31)
#define FRAGMENT_SIZE   (MAX_PACKETLEN - 100)

extern Cvar* sv_hostname;
extern loopback_t loopbacks[2];
extern int ip_sockets[2];
extern Cvar* sv_packetloss;
extern Cvar* sv_packetdelay;
extern Cvar* cl_packetloss;
extern Cvar* cl_packetdelay;
extern Cvar* showpackets;
extern const char* netsrcString[2];
extern Cvar* qport;
extern Cvar* showdrop;

int NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
void NET_SendLoopPacket(netsrc_t sock, int length, const void* data, int type);
bool NET_GetUdpPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
bool NET_GetPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
void NET_SendPacket(netsrc_t sock, int length, const void* data, const netadr_t& to);
void NET_OutOfBandPrint(netsrc_t sock, const netadr_t& adr, const char* format, ...) id_attribute((format(printf, 3, 4)));
void NET_OutOfBandData(netsrc_t sock, const netadr_t& adr, const byte* data, int length);
void Netchan_TransmitNextFragment(netchan_t* chan);
void Netchan_Transmit(netchan_t* chan, int length, const byte* data);
bool Netchan_CanReliable(netchan_t* chan);
bool Netchan_Process(netchan_t* chan, QMsg* msg);

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

extern int hostCacheCount;
extern hostcache_t hostcache[HOSTCACHESIZE];
extern int net_activeconnections;
extern int net_numsockets;
extern double net_time;
extern bool tcpipAvailable;
extern int net_acceptsocket;
extern int net_hostport;
extern netadr_t banAddr;
extern netadr_t banMask;
extern int udp_controlSock;
extern bool udp_initialized;
extern bool datagram_initialized;
extern bool net_listening;
extern int DEFAULTnet_hostport;

qsocket_t* NET_NewQSocket();
void NET_FreeQSocket(qsocket_t*);
double SetNetTime();
void Loop_SearchForHosts(bool xmit);
qsocket_t* Loop_Connect(const char* host, netchan_t* chan);
qsocket_t* Loop_CheckNewConnections(netadr_t* outaddr);
int  UDPNQ_OpenSocket(int port);
int  UDP_Read(int socket, byte* buf, int len, netadr_t* addr);
int  UDP_Write(int socket, byte* buf, int len, netadr_t* addr);
int  UDP_Broadcast(int socket, byte* buf, int len);
int  UDP_GetNameFromAddr(netadr_t* addr, char* name);
int  UDP_GetAddrFromName(const char* name, netadr_t* addr);
int  UDP_AddrCompare(netadr_t* addr1, netadr_t* addr2);
void NET_Ban_f();
// if a dead connection is returned by a get or send function, this function
// should be called when it is convenient
// Server calls when a client is kicked off for a game related misbehavior
// like an illegal protocal conversation.  Client calls when disconnecting
// from a server.
// A netcon_t number will not be reused until this function is called for it
void NET_Close(qsocket_t* sock, netchan_t* chan);
// returns data in net_message sizebuf
// returns 0 if no data is waiting
// returns 1 if a message was received
// returns 2 if an unreliable message was received
// returns -1 if the connection died
int NET_GetMessage(qsocket_t* sock, netchan_t* chan, QMsg* message);
// returns 0 if the message connot be delivered reliably, but the connection
//		is still considered valid
// returns 1 if the message was sent properly
// returns -1 if the connection died
int NET_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
int NET_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data);
// Returns true or false if the given qsocket can currently accept a
// message to be transmitted.
bool NET_CanSendMessage(qsocket_t* sock, netchan_t* chan);
void NET_CommonInit();
void NETQH_Shutdown();
