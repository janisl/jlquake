/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// net_udp.c

#include "quakedef.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#if defined(sun)
#include <unistd.h>
#endif

#ifdef sun
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#if _DEBUG
extern int HuffIn;
extern int HuffOut;
void ZeroFreq();
void CalcFreq(unsigned char *packet, int packetlen);
void PrintFreqs();
#endif
extern float HuffFreq[256];
void BuildTree(float *freq);
void HuffDecode(unsigned char *in,unsigned char *out,int inlen,int *outlen);
void HuffEncode(unsigned char *in,unsigned char *out,int inlen,int *outlen);

int LastCompMessageSize = 0;

netadr_t	net_local_adr;

netadr_t	net_from;
QMsg		net_message;
int			net_socket;			// non blocking, for receives

#define	MAX_UDP_PACKET	(MAX_MSGLEN+9)	// one more than msg + header
byte		net_message_buffer[MAX_UDP_PACKET];

//=============================================================================

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	sprintf (s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}

char	*NET_BaseAdrToString (netadr_t a)
{
	static	char	s[64];
	
	sprintf (s, "%i.%i.%i.%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3]);

	return s;
}

/*
=============
NET_StringToAdr

idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	struct sockaddr_in sadr;
	
	if (!SOCK_StringToSockaddr(s, &sadr))
		return false;
	
	SockadrToNetadr (&sadr, a);

	return true;
}

//=============================================================================
static unsigned char huffbuff[65536];

qboolean NET_GetPacket (void)
{
	int ret = SOCK_Recv(net_socket, huffbuff, sizeof(net_message_buffer), &net_from);
	if (ret == SOCKRECV_NO_DATA)
	{
		return false;
	}
	if (ret == SOCKRECV_ERROR)
	{
		return false;
	}

	if (ret == sizeof(net_message_buffer) )
	{
		Con_Printf ("Oversize packet from %s\n", NET_AdrToString (net_from));
		return false;
	}

	LastCompMessageSize += ret;//keep track of bytes actually received for debugging

	HuffDecode(huffbuff, (unsigned char *)net_message_buffer,ret,&ret);
	net_message.cursize = ret;

	return ret;
}

//=============================================================================

void NET_SendPacket (int length, void *data, netadr_t to)
{
	int outlen;

	HuffEncode((unsigned char *)data, huffbuff, length, &outlen);

	SOCL_Send(net_socket, huffbuff, outlen, &to);
}

//=============================================================================

int UDP_OpenSocket (int port)
{
	int newsocket;
	int i;

	const char* net_interface = NULL;
	if ((i = COM_CheckParm("-ip")) != 0 && i < COM_Argc())
	{
		net_interface = COM_Argv(i+1);
	}
	newsocket = SOCK_Open(net_interface, port);
	if (newsocket == 0)
		Sys_Error ("UDP_OpenSocket: failed");
	return newsocket;
}

void NET_GetLocalAddress (void)
{
	char	buff[MAXHOSTNAMELEN];
	struct sockaddr_in	address;
	socklen_t	namelen;

#ifdef _DEBUG
	ZeroFreq();
#endif

	BuildTree(HuffFreq);

	gethostname(buff, MAXHOSTNAMELEN);
	buff[MAXHOSTNAMELEN-1] = 0;

	NET_StringToAdr (buff, &net_local_adr);

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error ("NET_Init: getsockname:", SOCK_ErrorString());
	net_local_adr.port = address.sin_port;

	Con_Printf("IP address %s\n", NET_AdrToString (net_local_adr) );
}

/*
====================
NET_Init
====================
*/
void NET_Init (int port)
{
	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket (port);

	//
	// init the message buffer
	//
	net_message.InitOOB(net_message_buffer, sizeof(net_message_buffer));

	//
	// determine my name & address
	//
	NET_GetLocalAddress ();

	Con_Printf("UDP Initialized\n");
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	SOCK_Close(net_socket);

#ifdef _DEBUG
	PrintFreqs();
#endif
}

