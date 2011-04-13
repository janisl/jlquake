// net_wins.c

#include "quakedef.h"
#include "winquake.h"

extern QCvar* hostname;

extern int net_acceptsocket;		// socket for fielding new connections
extern int net_controlsocket;

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
