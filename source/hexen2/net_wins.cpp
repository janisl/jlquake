// net_wins.c

#include "quakedef.h"
#include "winquake.h"

extern int net_acceptsocket;		// socket for fielding new connections

#include "net_udp.h"

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
