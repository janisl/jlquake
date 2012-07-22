/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../game/q_shared.h"
#include "qcommon.h"

/*

packet header
-------------
4	outgoing sequence.  high bit will be set if this is a fragmented message
[2	qport (only for client to server)]
[2	fragment start byte]
[2	fragment length. if < FRAGMENT_SIZE, this is the last fragment]

if the sequence number is -1, the packet should be handled as an out-of-band
message instead of as part of a netcon.

All fragments will have the same sequence numbers.

The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.

*/

static qboolean networkingEnabled = qfalse;

static Cvar* net_noudp;

/*
====================
NET_OpenIP
====================
*/
static void NET_OpenIP()
{
	Cvar* ip = Cvar_Get("net_ip", "localhost", CVAR_LATCH2);
	int port = Cvar_Get("net_port", va("%i", PORT_SERVER), CVAR_LATCH2)->integer;

	// automatically scan for a valid port, so multiple
	// dedicated servers can be started without requiring
	// a different net_port for each one
	for (int i = 0; i < 10; i++)
	{
		ip_sockets[0] = SOCK_Open(ip->string, port + i);
		if (ip_sockets[0])
		{
			ip_sockets[1] = ip_sockets[0];
			Cvar_SetValue("net_port", port + i);
			if (net_socksEnabled->integer)
			{
				SOCK_OpenSocks(port + i);
			}
			SOCK_GetLocalAddress();
			return;
		}
	}
	Com_Printf("WARNING: Couldn't allocate IP port\n");
}

//===================================================================


/*
====================
NET_GetCvars
====================
*/
static bool NET_GetCvars()
{
	bool modified = false;

	if (net_noudp && net_noudp->modified)
	{
		modified = true;
	}
	net_noudp = Cvar_Get("net_noudp", "0", CVAR_LATCH2 | CVAR_ARCHIVE);

	if (SOCK_GetSocksCvars())
	{
		modified = true;
	}

	return modified;
}

/*
====================
NET_Config
====================
*/
void NET_Config(bool enableNetworking)
{
	bool modified;
	bool stop;
	bool start;

	// get any latched changes to cvars
	modified = NET_GetCvars();

	if (net_noudp->integer)
	{
		enableNetworking = false;
	}

	// if enable state is the same and no cvars were modified, we have nothing to do
	if (enableNetworking == networkingEnabled && !modified)
	{
		return;
	}

	if (enableNetworking == networkingEnabled)
	{
		if (enableNetworking)
		{
			stop = true;
			start = true;
		}
		else
		{
			stop = false;
			start = false;
		}
	}
	else
	{
		if (enableNetworking)
		{
			stop = false;
			start = true;
		}
		else
		{
			stop = true;
			start = false;
		}
		networkingEnabled = enableNetworking;
	}

	if (stop)
	{
		if (ip_sockets[0])
		{
			SOCK_Close(ip_sockets[0]);
			ip_sockets[0] = 0;
		}

		SOCK_CloseSocks();
	}

	if (start)
	{
		NET_OpenIP();
	}
}

/*
====================
NET_Init
====================
*/
void NET_Init()
{
	if (!SOCK_Init())
	{
		return;
	}

	// this is really just to get the cvars registered
	NET_GetCvars();

	//FIXME testing!
	NET_Config(true);
}

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown()
{
	NET_Config(false);
}

/*
====================
NET_Restart_f
====================
*/
void NET_Restart()
{
	NET_Config(networkingEnabled);
}

// sleeps msec or until net socket is ready
//JL: Note on Windows this  method was empty.
void NET_Sleep(int msec)
{
	if (!com_dedicated->integer)
	{
		return;	// we're not a server, just run full speed
	}

	SOCK_Sleep(ip_sockets[0], msec);
}
