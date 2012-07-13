// net_loop.c

#include "quakedef.h"
#include "net_loop.h"

#define MAX_LOOPBACK    16

struct loopmsg_t
{
	byte data[MAX_MSGLEN_H2];
	int datalen;
	byte type;
};

typedef struct
{
	loopmsg_t msgs[MAX_LOOPBACK];
	int get, send;
} loopback_t;

loopback_t loopbacks[2];
qboolean localconnectpending = false;
qsocket_t* loop_client = NULL;
qsocket_t* loop_server = NULL;

int Loop_Init(void)
{
	if (cls.state == CA_DEDICATED)
	{
		return -1;
	}
	return 0;
}


void Loop_Shutdown(void)
{
}


void Loop_Listen(qboolean state)
{
}


void Loop_SearchForHosts(qboolean xmit)
{
	if (sv.state == SS_DEAD)
	{
		return;
	}

	hostCacheCount = 1;
	if (String::Cmp(hostname->string, "UNNAMED") == 0)
	{
		String::Cpy(hostcache[0].name, "local");
	}
	else
	{
		String::Cpy(hostcache[0].name, hostname->string);
	}
	String::Cpy(hostcache[0].map, sv.name);
	hostcache[0].users = net_activeconnections;
	hostcache[0].maxusers = svs.qh_maxclients;
	hostcache[0].driver = net_driverlevel;
	String::Cpy(hostcache[0].cname, "local");
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
			Con_Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_client->address, "localhost");
	}
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loop_client->canSend = true;

	if (!loop_server)
	{
		if ((loop_server = NET_NewQSocket()) == NULL)
		{
			Con_Printf("Loop_Connect: no qsocket available\n");
			return NULL;
		}
		String::Cpy(loop_server->address, "LOCAL");
	}
	loopbacks[1].send = 0;
	loopbacks[1].get = 0;
	loop_server->canSend = true;

	loop_client->driverdata = (void*)loop_server;
	loop_server->driverdata = (void*)loop_client;

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
	loop_server->canSend = true;
	loopbacks[0].send = 0;
	loopbacks[0].get = 0;
	loop_client->canSend = true;
	return loop_server;
}

int Loop_GetMessage(qsocket_t* sock, netchan_t* chan)
{
	int ret;
	int length;

	loopback_t* loop = &loopbacks[chan->sock];
	if (loop->send - loop->get > MAX_LOOPBACK)
	{
		loop->get = loop->send - MAX_LOOPBACK;
	}

	if (loop->get >= loop->send)
	{
		return 0;
	}

	int i = loop->get & (MAX_LOOPBACK - 1);
	ret = loop->msgs[i].type;
	length = loop->msgs[i].datalen;
	// alignment byte skipped here
	net_message.Clear();
	net_message.WriteData(loop->msgs[i].data, length);
	loop->get++;

	if (sock->driverdata && ret == 1)
	{
		((qsocket_t*)sock->driverdata)->canSend = true;
	}

	return ret;
}


int Loop_SendMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	if (!sock->driverdata)
	{
		return -1;
	}

	if (data->cursize > MAX_MSGLEN_H2)
	{
		Sys_Error("Loop_SendMessage: overflow\n");
	}

	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	loopmsg_t* loop = &loopback->msgs[loopback->send++ & (MAX_LOOPBACK - 1)];
	loop->type = 1;
	loop->datalen = data->cursize;
	Com_Memcpy(loop->data, data->_data, data->cursize);

	sock->canSend = false;
	return 1;
}


int Loop_SendUnreliableMessage(qsocket_t* sock, netchan_t* chan, QMsg* data)
{
	if (!sock->driverdata)
	{
		return -1;
	}

	loopback_t* loopback = &loopbacks[chan->sock ^ 1];
	if (loopback->send - loopback->get >= MAX_LOOPBACK)
	{
		return 0;
	}

	loopmsg_t* loop = &loopback->msgs[loopback->send++ & (MAX_LOOPBACK - 1)];
	loop->type = 2;
	loop->datalen = data->cursize;
	Com_Memcpy(loop->data, data->_data, data->cursize);
	return 1;
}


qboolean Loop_CanSendMessage(qsocket_t* sock, netchan_t* chan)
{
	if (!sock->driverdata)
	{
		return false;
	}
	return sock->canSend;
}


qboolean Loop_CanSendUnreliableMessage(qsocket_t* sock)
{
	return true;
}


void Loop_Close(qsocket_t* sock, netchan_t* chan)
{
	if (sock->driverdata)
	{
		((qsocket_t*)sock->driverdata)->driverdata = NULL;
	}
	loopbacks[chan->sock].send = 0;
	loopbacks[chan->sock].get = 0;
	sock->canSend = true;
	if (sock == loop_client)
	{
		loop_client = NULL;
	}
	else
	{
		loop_server = NULL;
	}
}
