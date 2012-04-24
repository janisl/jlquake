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

#define MAX_MSGLEN_Q1		8192		// max length of a reliable message
#define MAX_MSGLEN_QW		1450		// max length of a reliable message
#define MAX_MSGLEN_H2		16384
#define MAX_MSGLEN_HW		7500		// max length of a reliable message
#define MAX_MSGLEN_Q2		1400		// max length of a message
#define MAX_MSGLEN_Q3		16384		// max length of a message, which may
										// be fragmented into multiple packets
#define MAX_MSGLEN_WOLF		32768		// increased for larger submodel entity counts
#define MAX_MSGLEN			32768		// biggest value of all of the above

#define MAX_DATAGRAM_Q1		1024		// max length of unreliable message
#define MAX_DATAGRAM_QW		1450		// max length of unreliable message
#define MAX_DATAGRAM_H2		1024		// max length of unreliable message
#define MAX_DATAGRAM_HW		1400		// max length of unreliable message
#define MAX_DATAGRAM		1450		// biggest value of all of the above

#define MAX_PACKETLEN		1400		// max size of a network packet

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

#define NET_NAMELEN_Q1			64

struct qsocket_t
{
	qsocket_t*		next;
	double			connecttime;

	qboolean		disconnected;
	qboolean		canSend;
	qboolean		sendNext;
	
	int				driver;
	int				socket;
	void			*driverdata;

	char			address[NET_NAMELEN_Q1];
};
