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

#include "network_channel.h"
#include "../../client_main.h"
#include "../../ui/ui.h"
#include "menu.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/system.h"
#include "../../../common/endian.h"

struct PollProcedure
{
	PollProcedure* next;
	double nextTime;
	void (* procedure)();
	void* arg;
};

bool slistInProgress = false;
bool slistSilent = false;
bool slistLocal = true;

static int slistLastShown;
static double slistStartTime;

static void Slist_Send();
static void Slist_Poll();

static PollProcedure slistSendProcedure = {NULL, 0.0, Slist_Send};
static PollProcedure slistPollProcedure = {NULL, 0.0, Slist_Poll};

static PollProcedure* pollProcedureList = NULL;

static void Datagram_SearchForHosts(bool xmit)
{
	if (!udp_initialized)
	{
		return;
	}

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

	int ret;
	netadr_t readaddr;
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
		int control = BigLong(*((int*)net_message._data));
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
		int n;
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
		for (int i = 0; i < hostCacheCount; i++)
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

void NET_Poll()
{
	PollProcedure* pp;

	SetNetTime();

	for (pp = pollProcedureList; pp; pp = pp->next)
	{
		if (pp->nextTime > net_time)
		{
			break;
		}
		pollProcedureList = pp->next;
		pp->procedure();
	}
}

static void SchedulePollProcedure(PollProcedure* proc, double timeOffset)
{
	PollProcedure* pp, * prev;

	proc->nextTime = Sys_DoubleTime() + timeOffset;
	for (pp = pollProcedureList, prev = NULL; pp; pp = pp->next)
	{
		if (pp->nextTime >= proc->nextTime)
		{
			break;
		}
		prev = pp;
	}

	if (prev == NULL)
	{
		proc->next = pollProcedureList;
		pollProcedureList = proc;
		return;
	}

	proc->next = pp;
	prev->next = proc;
}

static void PrintSlistHeader()
{
	common->Printf("Server          Map             Users\n");
	common->Printf("--------------- --------------- -----\n");
	slistLastShown = 0;
}

static void PrintSlist()
{
	int n;
	for (n = slistLastShown; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
		{
			common->Printf("%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		}
		else
		{
			common->Printf("%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		}
	}
	slistLastShown = n;
}

static void PrintSlistTrailer()
{
	if (hostCacheCount)
	{
		common->Printf("== end list ==\n\n");
	}
	else
	{
		common->Printf(GGameType & GAME_Hexen2 ? "No Hexen II servers found.\n\n" : "No Quake servers found.\n\n");
	}
}

static void Slist_Send()
{
	if (slistLocal)
	{
		Loop_SearchForHosts(true);
	}
	if (datagram_initialized)
	{
		Datagram_SearchForHosts(true);
	}

	if ((Sys_DoubleTime() - slistStartTime) < 0.5)
	{
		SchedulePollProcedure(&slistSendProcedure, 0.75);
	}
}

static void Slist_Poll()
{
	if (slistLocal)
	{
		Loop_SearchForHosts(false);
	}
	if (datagram_initialized)
	{
		Datagram_SearchForHosts(false);
	}

	if (!slistSilent)
	{
		PrintSlist();
	}

	if ((Sys_DoubleTime() - slistStartTime) < 1.5)
	{
		SchedulePollProcedure(&slistPollProcedure, 0.1);
		return;
	}

	if (!slistSilent)
	{
		PrintSlistTrailer();
	}
	slistInProgress = false;
	slistSilent = false;
	slistLocal = true;
}

void NET_Slist_f()
{
	if (slistInProgress)
	{
		return;
	}

	if (!slistSilent)
	{
		common->Printf(GGameType& GAME_Hexen2 ? "Looking for Hexen II servers...\n" : "Looking for Quake servers...\n");
		PrintSlistHeader();
	}

	slistInProgress = true;
	slistStartTime = Sys_DoubleTime();

	SchedulePollProcedure(&slistSendProcedure, 0.0);
	SchedulePollProcedure(&slistPollProcedure, 0.1);

	hostCacheCount = 0;
}

static bool Datagram_Connect(const char* host, netchan_t* chan)
{
	if (!udp_initialized)
	{
		return false;
	}

	// see if we can resolve the host name
	netadr_t sendaddr;
	if (UDP_GetAddrFromName(host, &sendaddr) == -1)
	{
		return false;
	}

	int newsock = UDPNQ_OpenSocket(0);
	if (newsock == -1)
	{
		return false;
	}

	double start_time;
	int ret;
	chan->socket = newsock;

	// send the connection request
	common->Printf("trying...\n"); SCR_UpdateScreen();
	start_time = net_time;

	QMsg net_message;
	byte net_message_buf[MAX_DATAGRAM_QH];
	net_message.InitOOB(net_message_buf, MAX_DATAGRAM_QH);

	for (int reps = 0; reps < 3; reps++)
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
			netadr_t readaddr;
			ret = UDP_Read(newsock, net_message._data, net_message.maxsize, &readaddr);
			// if we got something, validate it
			if (ret > 0)
			{
				// is it from the right place?
				if (UDP_AddrCompare(&readaddr, &sendaddr) != 0)
				{
#ifdef DEBUG
					common->Printf("wrong reply address\n");
					common->Printf("Expected: %s\n", SOCK_AdrToString(sendaddr));
					common->Printf("Received: %s\n", SOCK_AdrToString(readaddr));
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

				int control = BigLong(*((int*)net_message._data));
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
		const char* reason = "No Response";
		common->Printf("%s\n", reason);
		String::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	if (ret == -1)
	{
		const char* reason = "Network Error";
		common->Printf("%s\n", reason);
		String::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	ret = net_message.ReadByte();
	if (ret == CCREP_REJECT)
	{
		const char* reason = net_message.ReadString2();
		common->Printf("%s", reason);
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
		const char* reason = "Bad Response";
		common->Printf("%s\n", reason);
		String::Cpy(m_return_reason, reason);
		goto ErrorReturn;
	}

	common->Printf("Connection accepted\n");
	chan->lastReceived = SetNetTime() * 1000;

	m_return_onerror = false;
	return true;

ErrorReturn:
	UDP_CloseSocket(newsock);
	if (m_return_onerror)
	{
		in_keyCatchers |= KEYCATCH_UI;
		m_state = m_return_state;
		m_return_onerror = false;
	}
	return false;
}

bool NET_Connect(const char* host, netchan_t* chan)
{
	SetNetTime();

	if (host && *host == 0)
	{
		host = NULL;
	}

	int numdrivers = 2;
	if (host)
	{
		if (String::ICmp(host, "local") == 0)
		{
			numdrivers = 1;
			goto JustDoIt;
		}

		if (hostCacheCount)
		{
			int n;
			for (n = 0; n < hostCacheCount; n++)
			{
				if (String::ICmp(host, hostcache[n].name) == 0)
				{
					host = hostcache[n].cname;
					break;
				}
			}
			if (n < hostCacheCount)
			{
				goto JustDoIt;
			}
		}
	}

	slistSilent = host ? true : false;
	NET_Slist_f();

	while (slistInProgress)
	{
		NET_Poll();
	}

	if (host == NULL)
	{
		if (hostCacheCount != 1)
		{
			return NULL;
		}
		host = hostcache[0].cname;
		common->Printf("Connecting to...\n%s @ %s\n\n", hostcache[0].name, host);
	}

	if (hostCacheCount)
	{
		for (int n = 0; n < hostCacheCount; n++)
		{
			if (String::ICmp(host, hostcache[n].name) == 0)
			{
				host = hostcache[n].cname;
				break;
			}
		}
	}

JustDoIt:
	bool ret = Loop_Connect(host, chan);
	if (ret)
	{
		return ret;
	}

	if (datagram_initialized && numdrivers > 1)
	{
		ret = Datagram_Connect(host, chan);
		if (ret)
		{
			return ret;
		}
	}

	if (host)
	{
		common->Printf("\n");
		PrintSlistHeader();
		PrintSlist();
		PrintSlistTrailer();
	}

	return NULL;
}

void CLQH_ShutdownNetwork()
{
	NET_Close(&clc.netchan);
}
