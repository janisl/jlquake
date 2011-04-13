// net_wins.c

#include "quakedef.h"
#include "winquake.h"

extern QCvar* hostname;

extern int net_acceptsocket;		// socket for fielding new connections
extern int net_controlsocket;

static unsigned long myAddr;

#include "net_udp.h"

//=============================================================================

int UDP_Init (void)
{
	int		i;
	netadr_t addr;
	char	*p;

	if (COM_CheckParm ("-noudp"))
		return -1;

	if (!SOCK_Init())
	{
		return -1;
	}

	// determine my name & address
	SOCK_GetLocalAddress();

	myAddr = *(int *)localIP[0];

	if ((net_controlsocket = UDP_OpenSocket (PORT_ANY)) == -1)
	{
		Con_Printf("UDP_Init: Unable to open control socket\n");
		SOCK_Shutdown();
		return -1;
	}

	UDP_GetSocketAddr (net_controlsocket, &addr);
	QStr::Cpy(my_tcpip_address,  UDP_AddrToString (&addr));
	p = QStr::RChr(my_tcpip_address, ':');
	if (p)
		*p = 0;

	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

int UDP_CheckNewConnections (void)
{
	char buf[4096];

	if (net_acceptsocket == -1)
		return -1;

	if (recvfrom (net_acceptsocket, buf, sizeof(buf), MSG_PEEK, NULL, NULL) > 0)
	{
		return net_acceptsocket;
	}
	return -1;
}

//=============================================================================

int UDP_GetSocketAddr(int socket, netadr_t* addr)
{
	SOCK_GetAddr(socket, addr);
	if ((addr->ip[0] == 0 && addr->ip[1] == 0 && addr->ip[2] == 0 && addr->ip[3] == 0) ||
		(addr->ip[0] == 127 && addr->ip[1] == 0 && addr->ip[2] == 0 && addr->ip[3] == 1))
	{
		addr->ip[0] = localIP[0][0];
		addr->ip[1] = localIP[0][1];
		addr->ip[2] = localIP[0][2];
		addr->ip[3] = localIP[0][3];
	}
	return 0;
}

//=============================================================================

int UDP_GetNameFromAddr(netadr_t* addr, char* name)
{
	sockaddr_in sadr;
	NetadrToSockadr(addr, &sadr);
	struct hostent *hostentry;

	hostentry = gethostbyaddr ((char *)&sadr.sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		QStr::NCpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	QStr::Cpy(name, UDP_AddrToString(addr));
	return 0;
}
