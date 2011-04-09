/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_wins.c

#include "winsock.h"
#include "../qcommon/qcommon.h"

QCvar		*net_shownet;
static QCvar	*noudp;

extern int			ip_sockets[2];

//=============================================================================

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
			return true;
	}
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
		return TRUE;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
	}
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];

	if (a.type == NA_LOOPBACK)
		QStr::Sprintf (s, sizeof(s), "loopback");
	else
		QStr::Sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

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
	struct sockaddr sadr;
	
	if (!QStr::Cmp(s, "localhost"))
	{
		Com_Memset(a, 0, sizeof(*a));
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!SOCK_StringToSockaddr (s, (struct sockaddr_in*)&sadr))
		return false;
	
	SockadrToNetadr ((struct sockaddr_in*)&sadr, a);

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
	QCvar	*ip;
	int		port;
	int		dedicated;

	ip = Cvar_Get ("ip", "localhost", CVAR_INIT);

	dedicated = Cvar_VariableValue ("dedicated");

	if (!ip_sockets[NS_SERVER])
	{
		port = Cvar_Get("ip_hostport", "0", CVAR_INIT)->value;
		if (!port)
		{
			port = Cvar_Get("hostport", "0", CVAR_INIT)->value;
			if (!port)
			{
				port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_INIT)->value;
			}
		}
		ip_sockets[NS_SERVER] = SOCK_Open (ip->string, port);
		if (!ip_sockets[NS_SERVER] && dedicated)
			Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port");
	}


	// dedicated servers don't need client ports
	if (dedicated)
		return;

	if (!ip_sockets[NS_CLIENT])
	{
		port = Cvar_Get("ip_clientport", "0", CVAR_INIT)->value;
		if (!port)
		{
			port = Cvar_Get("clientport", va("%i", PORT_CLIENT), CVAR_INIT)->value;
			if (!port)
				port = PORT_ANY;
		}
		ip_sockets[NS_CLIENT] = SOCK_Open (ip->string, port);
		if (!ip_sockets[NS_CLIENT])
			ip_sockets[NS_CLIENT] = SOCK_Open (ip->string, PORT_ANY);
	}
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
	static	qboolean	old_config;

	if (old_config == multiplayer)
		return;

	old_config = multiplayer;

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
		if (! noudp->value)
			NET_OpenIP ();
	}
}

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
    struct timeval timeout;
	fd_set	fdset;
	extern QCvar *dedicated;
	int i;

	if (!dedicated || !dedicated->value)
		return; // we're not a server, just run full speed

	FD_ZERO(&fdset);
	i = 0;
	if (ip_sockets[NS_SERVER]) {
		FD_SET(ip_sockets[NS_SERVER], &fdset); // network socket
		i = ip_sockets[NS_SERVER];
	}
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select(i+1, &fdset, NULL, NULL, &timeout);
}

//===================================================================

/*
====================
NET_Init
====================
*/
void NET_Init (void)
{
	if (!SOCK_Init())
		Com_Error (ERR_FATAL,"Sockets initialization failed.");

	noudp = Cvar_Get ("noudp", "0", CVAR_INIT);

	net_shownet = Cvar_Get ("net_shownet", "0", 0);
}


/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	NET_Config (false);	// close sockets

	SOCK_Shutdown();
}
