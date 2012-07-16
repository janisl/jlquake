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

#include "../../client.h"
#include "menu.h"

static qsocket_t* _Datagram_Connect(const char* host, netchan_t* chan)
{
	netadr_t sendaddr;
	netadr_t readaddr;
	qsocket_t* sock;
	int newsock;
	int ret;
	int reps;
	double start_time;
	int control;
	const char* reason;

	// see if we can resolve the host name
	if (UDP_GetAddrFromName(host, &sendaddr) == -1)
	{
		return NULL;
	}

	newsock = UDPNQ_OpenSocket(0);
	if (newsock == -1)
	{
		return NULL;
	}

	sock = NET_NewQSocket();
	if (sock == NULL)
	{
		goto ErrorReturn2;
	}
	sock->socket = newsock;

	// send the connection request
	common->Printf("trying...\n"); SCR_UpdateScreen();
	start_time = net_time;

	QMsg net_message;
	byte net_message_buf[MAX_DATAGRAM_QH];
	net_message.InitOOB(net_message_buf, MAX_DATAGRAM_QH);

	for (reps = 0; reps < 3; reps++)
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREQ_CONNECT);
		net_message.WriteString2(GGameType & GAME_Hexen2 ? NET_NAME_ID : "QUAKE");
		net_message.WriteByte(GGameType & GAME_Hexen2 ? H2NET_PROTOCOL_VERSION : Q1NET_PROTOCOL_VERSION);
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(newsock, net_message._data, net_message.cursize, &sendaddr);
		net_message.Clear();
		do
		{
			ret = UDP_Read(newsock, net_message._data, net_message.maxsize, &readaddr);
			// if we got something, validate it
			if (ret > 0)
			{
				// is it from the right place?
				if (UDP_AddrCompare(&readaddr, &sendaddr) != 0)
				{
#ifdef DEBUG
					Con_Printf("wrong reply address\n");
					Con_Printf("Expected: %s\n", SOCK_AdrToString(sendaddr));
					Con_Printf("Received: %s\n", SOCK_AdrToString(readaddr));
					SCR_UpdateScreen();
#endif
					ret = 0;
					continue;
				}

				if (ret < (int)sizeof(int))
				{
					ret = 0;
					continue;
				}

				net_message.cursize = ret;
				net_message.BeginReadingOOB();

				control = BigLong(*((int*)net_message._data));
				net_message.ReadLong();
				if (control == -1)
				{
					ret = 0;
					continue;
				}
				if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL)
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
		{
			break;
		}
		common->Printf("still trying...\n"); SCR_UpdateScreen();
		start_time = SetNetTime();
	}

	if (ret == 0)
	{
		reason = "No Response";
		common->Printf("%s\n", reason);
		String::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	if (ret == -1)
	{
		reason = "Network Error";
		common->Printf("%s\n", reason);
		String::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	ret = net_message.ReadByte();
	if (ret == CCREP_REJECT)
	{
		reason = net_message.ReadString2();
		common->Printf(reason);
		String::NCpy(m_return_reason, reason, 31);
		goto ErrorReturn;
	}

	if (ret == CCREP_ACCEPT)
	{
		chan->remoteAddress = sendaddr;
		SOCK_SetPort(&chan->remoteAddress, net_message.ReadLong());
	}
	else
	{
		reason = "Bad Response";
		common->Printf("%s\n", reason);
		String::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	UDP_GetNameFromAddr(&sendaddr, sock->address);

	common->Printf("Connection accepted\n");
	chan->lastReceived = SetNetTime() * 1000;

	m_return_onerror = false;
	return sock;

ErrorReturn:
	NET_FreeQSocket(sock);
ErrorReturn2:
	UDPNQ_OpenSocket(newsock);
	if (m_return_onerror)
	{
		in_keyCatchers |= KEYCATCH_UI;
		m_state = m_return_state;
		m_return_onerror = false;
	}
	return NULL;
}

qsocket_t* Datagram_Connect(const char* host, netchan_t* chan)
{
	qsocket_t* ret = NULL;

	if (udp_initialized)
	{
		ret = _Datagram_Connect(host, chan);
	}
	return ret;
}
