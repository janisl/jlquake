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
	struct qsockaddr addr;
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

char *UDP_AddrToString (struct qsockaddr *addr)
{
	static char buffer[22];
	int haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff, ntohs(((struct sockaddr_in *)addr)->sin_port));
	return buffer;
}

//=============================================================================

int UDP_GetSocketAddr (int socket, struct qsockaddr *addr)
{
	int addrlen = sizeof(struct qsockaddr);
	unsigned int a;

	Com_Memset(addr, 0, sizeof(struct qsockaddr));
	getsockname(socket, (struct sockaddr *)addr, &addrlen);
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	if (a == 0 || a == inet_addr("127.0.0.1"))
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

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

int UDP_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	if (!SOCK_StringToSockaddr(name, (struct sockaddr_in*)addr))
		return -1;

	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)net_hostport);	

	return 0;
}

//=============================================================================

int UDP_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	if (addr1->sa_family != addr2->sa_family)
		return -1;

	if (((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
		return -1;

	if (((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port)
		return 1;

	return 0;
}

//=============================================================================

int UDP_GetSocketPort (struct qsockaddr *addr)
{
	return ntohs(((struct sockaddr_in *)addr)->sin_port);
}


int UDP_SetSocketPort (struct qsockaddr *addr, int port)
{
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)port);
	return 0;
}

//=============================================================================
