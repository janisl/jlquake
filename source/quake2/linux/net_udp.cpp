// net_wins.c

#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <arpa/inet.h>

#ifdef NeXT
#include <libc.h>
#endif

#define	LOOPBACK	0x7f000001

extern int			ip_sockets[2];

//=============================================================================

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
		return false;
	}
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	QStr::Sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	QStr::Sprintf (s, sizeof(s), "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	struct sockaddr_in sadr;
	
	if (!QStr::Cmp(s, "localhost"))
	{
		Com_Memset(a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!SOCK_StringToSockaddr (s, &sadr))
		return false;
	
	SockadrToNetadr (&sadr, a);

	return true;
}


qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return adr.type == NA_LOOPBACK;
}

//=============================================================================




/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (void)
{
	QCvar	*port, *ip;

	port = Cvar_Get ("port", va("%i", PORT_SERVER), CVAR_INIT);
	ip = Cvar_Get ("ip", "localhost", CVAR_INIT);

	if (!ip_sockets[NS_SERVER])
		ip_sockets[NS_SERVER] = SOCK_Open(ip->string, port->value);
	if (!ip_sockets[NS_CLIENT])
		ip_sockets[NS_CLIENT] = SOCK_Open(ip->string, PORT_ANY);
}

/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
void	NET_Config (qboolean multiplayer)
{
	int		i;

	if (!multiplayer)
	{	// shut down any existing sockets
		for (i=0 ; i<2 ; i++)
		{
			if (ip_sockets[i])
			{
				SOCK_Close(ip_sockets[i]);
				ip_sockets[i] = 0;
			}
		}
	}
	else
	{	// open sockets
		NET_OpenIP ();
	}
}


//===================================================================


/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	NET_Config (false);	// close sockets
}

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
    struct timeval timeout;
	fd_set	fdset;
	extern QCvar *dedicated;
	extern qboolean stdin_active;

	if (!ip_sockets[NS_SERVER] || (dedicated && !dedicated->value))
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	if (stdin_active)
		FD_SET(0, &fdset); // stdin is processed too
	FD_SET(ip_sockets[NS_SERVER], &fdset); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(ip_sockets[NS_SERVER]+1, &fdset, NULL, NULL, &timeout);
}

