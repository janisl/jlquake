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
	qsockaddr sadr;
	NetadrToSockadr(&addr, (struct sockaddr_in*)&sadr);
	QStr::Cpy(my_tcpip_address,  UDP_AddrToString (&sadr));
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
	sockaddr_in sadr;
	int addrlen = sizeof(sadr);
	unsigned int a;

	Com_Memset(&sadr, 0, sizeof(struct qsockaddr));
	getsockname(socket, (struct sockaddr *)&sadr, &addrlen);
	a = sadr.sin_addr.s_addr;
	if (a == 0 || a == inet_addr("127.0.0.1"))
		sadr.sin_addr.s_addr = myAddr;

	SockadrToNetadr(&sadr, addr);
	return 0;
}

//=============================================================================

int UDP_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
	struct hostent *hostentry;

	hostentry = gethostbyaddr ((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		QStr::NCpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	QStr::Cpy(name, UDP_AddrToString (addr));
	return 0;
}

//=============================================================================

int UDP_GetAddrFromName(const char *name, struct qsockaddr *addr)
{
	netadr_t a;
	sockaddr_in sadr;
	if (!SOCK_StringToSockaddr(name, &sadr))
		return -1;

	SockadrToNetadr(&sadr, &a);
	a.port = htons(net_hostport);
	NetadrToSockadr(&a, (struct sockaddr_in*)addr);

	return 0;
}
