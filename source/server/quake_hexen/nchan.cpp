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

#include "../server.h"
#include "local.h"

// check to see if client block will fit, if not, rotate buffers
void SVQH_ClientReliableCheckBlock(client_t* cl, int maxsize)
{
	//	Only in QuakeWorld.
	if (GGameType & GAME_Hexen2)
	{
		return;
	}

	if (cl->qw_num_backbuf ||
		cl->netchan.message.cursize >
		cl->netchan.message.maxsize - maxsize - 1)
	{
		// we would probably overflow the buffer, save it for next
		if (!cl->qw_num_backbuf)
		{
			cl->qw_backbuf.InitOOB(cl->qw_backbuf_data[0], sizeof(cl->qw_backbuf_data[0]));
			cl->qw_backbuf.allowoverflow = true;
			cl->qw_backbuf_size[0] = 0;
			cl->qw_num_backbuf++;
		}

		if (cl->qw_backbuf.cursize > cl->qw_backbuf.maxsize - maxsize - 1)
		{
			if (cl->qw_num_backbuf == MAX_BACK_BUFFERS)
			{
				common->Printf("WARNING: MAX_BACK_BUFFERS for %s\n", cl->name);
				cl->qw_backbuf.cursize = 0;// don't overflow without allowoverflow set
				cl->netchan.message.overflowed = true;	// this will drop the client
				return;
			}
			cl->qw_backbuf.InitOOB(cl->qw_backbuf_data[cl->qw_num_backbuf], sizeof(cl->qw_backbuf_data[cl->qw_num_backbuf]));
			cl->qw_backbuf.allowoverflow = true;
			cl->qw_backbuf_size[cl->qw_num_backbuf] = 0;
			cl->qw_num_backbuf++;
		}
	}
}

// begin a client block, estimated maximum size
void SVQH_ClientReliableWrite_Begin(client_t* cl, int c, int maxsize)
{
	SVQH_ClientReliableCheckBlock(cl, maxsize);
	SVQH_ClientReliableWrite_Byte(cl, c);
}

void SVQH_ClientReliable_FinishWrite(client_t* cl)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf_size[cl->qw_num_backbuf - 1] = cl->qw_backbuf.cursize;

		if (cl->qw_backbuf.overflowed)
		{
			common->Printf("WARNING: backbuf [%d] reliable overflow for %s\n",cl->qw_num_backbuf,cl->name);
			cl->netchan.message.overflowed = true;	// this will drop the client
		}
	}
}

void SVQH_ClientReliableWrite_Angle(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteAngle(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteAngle(f);
	}
}

void SVQH_ClientReliableWrite_Angle16(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteAngle16(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteAngle16(f);
	}
}

void SVQH_ClientReliableWrite_Byte(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteByte(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteByte(c);
	}
}

void SVQH_ClientReliableWrite_Char(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteChar(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteChar(c);
	}
}

void SVQH_ClientReliableWrite_Float(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteFloat(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteFloat(f);
	}
}

void SVQH_ClientReliableWrite_Coord(client_t* cl, float f)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteCoord(f);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteCoord(f);
	}
}

void SVQH_ClientReliableWrite_Long(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteLong(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteLong(c);
	}
}

void SVQH_ClientReliableWrite_Short(client_t* cl, int c)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteShort(c);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteShort(c);
	}
}

void SVQH_ClientReliableWrite_String(client_t* cl, const char* s)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteString2(s);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteString2(s);
	}
}

void SVQH_ClientReliableWrite_SZ(client_t* cl, const void* data, int len)
{
	if (cl->qw_num_backbuf)
	{
		cl->qw_backbuf.WriteData(data, len);
		SVQH_ClientReliable_FinishWrite(cl);
	}
	else
	{
		cl->netchan.message.WriteData(data, len);
	}
}

void Loop_SearchForHosts(bool xmit)
{
	if (sv.state == SS_DEAD)
	{
		return;
	}

	hostCacheCount = 1;
	if (String::Cmp(sv_hostname->string, "UNNAMED") == 0)
	{
		String::Cpy(hostcache[0].name, "local");
	}
	else
	{
		String::Cpy(hostcache[0].name, sv_hostname->string);
	}
	String::Cpy(hostcache[0].map, sv.name);
	hostcache[0].users = net_activeconnections;
	hostcache[0].maxusers = svs.qh_maxclients;
	hostcache[0].driver = 0;
	String::Cpy(hostcache[0].cname, "local");
}

void NET_Ban_f()
{
	char addrStr [32];
	char maskStr [32];

	if (sv.state == SS_DEAD)
	{
		return;
	}

	switch (Cmd_Argc())
	{
	case 1:
		if (banAddr.type == NA_IP)
		{
			String::Cpy(addrStr, SOCK_BaseAdrToString(banAddr));
			String::Cpy(maskStr, SOCK_BaseAdrToString(banMask));
			common->Printf("Banning %s [%s]\n", addrStr, maskStr);
		}
		else
		{
			common->Printf("Banning not active\n");
		}
		break;

	case 2:
		if (String::ICmp(Cmd_Argv(1), "off") == 0)
		{
			Com_Memset(&banAddr, 0, sizeof(banAddr));
		}
		else
		{
			UDP_GetAddrFromName(Cmd_Argv(1), &banAddr);
		}
		*(int*)banMask.ip = 0xffffffff;
		break;

	case 3:
		UDP_GetAddrFromName(Cmd_Argv(1), &banAddr);
		UDP_GetAddrFromName(Cmd_Argv(1), &banMask);
		break;

	default:
		common->Printf("BAN ip_address [mask]\n");
		break;
	}
}

qsocket_t* Datagram_CheckNewConnections(netadr_t* outaddr)
{
	if (!udp_initialized)
	{
		return NULL;
	}

	int acceptsock = net_acceptsocket;
	if (acceptsock == -1)
	{
		return NULL;
	}

	QMsg net_message;
	byte net_message_buf[MAX_DATAGRAM_QH];
	net_message.InitOOB(net_message_buf, MAX_DATAGRAM_QH);
	net_message.Clear();

	netadr_t clientaddr;
	int len = UDP_Read(acceptsock, net_message._data, net_message.maxsize, &clientaddr);
	if (len < (int)sizeof(int))
	{
		return NULL;
	}
	net_message.cursize = len;

	net_message.BeginReadingOOB();
	int control = BigLong(*((int*)net_message._data));
	net_message.ReadLong();
	if (control == -1)
	{
		return NULL;
	}
	if ((control & (~NETFLAG_LENGTH_MASK)) !=  (int)NETFLAG_CTL)
	{
		return NULL;
	}
	if ((control & NETFLAG_LENGTH_MASK) != len)
	{
		return NULL;
	}

	int command = net_message.ReadByte();
	if (command == CCREQ_SERVER_INFO)
	{
		if (String::Cmp(net_message.ReadString2(), GGameType & GAME_Hexen2 ? NET_NAME_ID : "QUAKE") != 0)
		{
			return NULL;
		}

		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_SERVER_INFO);
		netadr_t newaddr;
		SOCK_GetAddr(acceptsock, &newaddr);
		SOCK_CheckAddr(&newaddr);
		net_message.WriteString2(SOCK_AdrToString(newaddr));
		net_message.WriteString2(sv_hostname->string);
		net_message.WriteString2(sv.name);
		net_message.WriteByte(net_activeconnections);
		net_message.WriteByte(svs.qh_maxclients);
		net_message.WriteByte(GGameType & GAME_Hexen2 ? H2NET_PROTOCOL_VERSION : Q1NET_PROTOCOL_VERSION);
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
		net_message.Clear();
		return NULL;
	}

	if (command == CCREQ_PLAYER_INFO)
	{
		int playerNumber;
		int activeNumber;
		int clientNumber;
		client_t* client;

		playerNumber = net_message.ReadByte();
		activeNumber = -1;
		for (clientNumber = 0, client = svs.clients; clientNumber < svs.qh_maxclients; clientNumber++, client++)
		{
			if (client->state >= CS_CONNECTED)
			{
				activeNumber++;
				if (activeNumber == playerNumber)
				{
					break;
				}
			}
		}
		if (clientNumber == svs.qh_maxclients)
		{
			return NULL;
		}

		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_PLAYER_INFO);
		net_message.WriteByte(playerNumber);
		net_message.WriteString2(client->name);
		net_message.WriteLong(client->qh_colors);
		net_message.WriteLong((int)client->qh_edict->GetFrags());
		net_message.WriteLong((int)(net_time - client->qh_netconnection->connecttime));
		net_message.WriteString2(client->qh_netconnection->address);
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
		net_message.Clear();

		return NULL;
	}

	if (command == CCREQ_RULE_INFO)
	{
		const char* prevCvarName;
		Cvar* var;

		// find the search start location
		prevCvarName = net_message.ReadString2();
		if (*prevCvarName)
		{
			var = Cvar_FindVar(prevCvarName);
			if (!var)
			{
				return NULL;
			}
			var = var->next;
		}
		else
		{
			var = cvar_vars;
		}

		// search for the next server cvar
		while (var)
		{
			if (var->flags & CVAR_SERVERINFO)
			{
				break;
			}
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
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
		net_message.Clear();

		return NULL;
	}

	if (command != CCREQ_CONNECT)
	{
		return NULL;
	}

	if (String::Cmp(net_message.ReadString2(), GGameType & GAME_Hexen2 ? NET_NAME_ID : "QUAKE") != 0)
	{
		return NULL;
	}

	if (net_message.ReadByte() != (GGameType & GAME_Hexen2 ? H2NET_PROTOCOL_VERSION : Q1NET_PROTOCOL_VERSION))
	{
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_REJECT);
		net_message.WriteString2("Incompatible version.\n");
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
		net_message.Clear();
		return NULL;
	}

	// check for a ban
	if (clientaddr.type == NA_IP && banAddr.type == NA_IP)
	{
		quint32 testAddr = *(quint32*)clientaddr.ip;
		if ((testAddr & *(quint32*)banMask.ip) == *(quint32*)banAddr.ip)
		{
			net_message.Clear();
			// save space for the header, filled in later
			net_message.WriteLong(0);
			net_message.WriteByte(CCREP_REJECT);
			net_message.WriteString2("You have been banned.\n");
			*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
			UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
			net_message.Clear();
			return NULL;
		}
	}

	// see if this guy is already connected
	client_t* client = svs.clients;
	for (int i = 0; i < svs.qh_maxclients; i++, client++)
	{
		if (!client->qh_netconnection)
		{
			continue;
		}
		if (client->state < CS_CONNECTED)
		{
			continue;
		}
		qsocket_t* s = client->qh_netconnection;
		if (client->netchan.remoteAddress.type != NA_IP)
		{
			continue;
		}
		int ret = UDP_AddrCompare(&clientaddr, &client->netchan.remoteAddress);
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
				netadr_t newaddr;
				SOCK_GetAddr(s->socket, &newaddr);
				net_message.WriteLong(SOCK_GetPort(&newaddr));
				*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
				UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
				net_message.Clear();
				return NULL;
			}
			// it's somebody coming back in from a crash/disconnect
			// so close the old qsocket and let their retry get them back in
			NET_Close(s, &client->netchan);
			return NULL;
		}
	}

	if (net_activeconnections >= svs.qh_maxclients)
	{
		// no room; try to let him know
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_REJECT);
		net_message.WriteString2("Server is full.\n");
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
		net_message.Clear();
		return NULL;
	}

	// allocate a QSocket
	qsocket_t* sock = NET_NewQSocket();
	if (sock == NULL)
	{
		// no room; try to let him know
		net_message.Clear();
		// save space for the header, filled in later
		net_message.WriteLong(0);
		net_message.WriteByte(CCREP_REJECT);
		net_message.WriteString2("Server is full.\n");
		*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
		net_message.Clear();
		return NULL;
	}

	// allocate a network socket
	int newsock = UDPNQ_OpenSocket(0);
	if (newsock == -1)
	{
		NET_FreeQSocket(sock);
		return NULL;
	}

	// everything is allocated, just fill in the details
	sock->socket = newsock;
	*outaddr = clientaddr;
	String::Cpy(sock->address, SOCK_AdrToString(clientaddr));

	// send him back the info about the server connection he has been allocated
	net_message.Clear();
	// save space for the header, filled in later
	net_message.WriteLong(0);
	net_message.WriteByte(CCREP_ACCEPT);
	netadr_t newaddr;
	SOCK_GetAddr(newsock, &newaddr);
	net_message.WriteLong(SOCK_GetPort(&newaddr));
	*((int*)net_message._data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
	UDP_Write(acceptsock, net_message._data, net_message.cursize, &clientaddr);
	net_message.Clear();

	return sock;
}
