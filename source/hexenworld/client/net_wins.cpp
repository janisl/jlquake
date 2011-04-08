// net_wins.c

#include "quakedef.h"
#include "winquake.h"

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
int			net_socket;

#define	MAX_UDP_PACKET	(MAX_MSGLEN+9)	// one more than msg + header
byte		net_message_buffer[MAX_UDP_PACKET];

WSADATA		winsockdata;

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
	int 	ret;
	struct sockaddr_in	from;
	int		fromlen;

	fromlen = sizeof(from);
	ret = recvfrom (net_socket,(char *) huffbuff, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);
	if (ret == -1)
	{
		int err = WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
			return false;
		Sys_Error ("NET_GetPacket: %s", strerror(err));
	}

	SockadrToNetadr (&from, &net_from);

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
	int ret;
	int outlen;
	struct sockaddr_in	addr;
	char string[120];

	NetadrToSockadr (&to, &addr);
	HuffEncode((unsigned char *)data,huffbuff,length,&outlen);

#ifdef _DEBUG
	sprintf(string,"in: %d  out: %d  ratio: %f\n",HuffIn, HuffOut, 1-(float)HuffOut/(float)HuffIn);
	OutputDebugString(string);

	CalcFreq((unsigned char *)data, length);
#endif

	ret = sendto (net_socket, (char *) huffbuff, outlen, 0, (struct sockaddr *)&addr, sizeof(addr) );
	if (ret == -1)
	{
		int err = WSAGetLastError();

		Con_Printf ("NET_SendPacket ERROR: %i", err);
	}
}

//=============================================================================

int UDP_OpenSocket (int port)
{
	int newsocket = SOCK_Open(NULL, port);
	if (newsocket == 0)
		Sys_Error ("UDP_OpenSocket: socket failed");
	return newsocket;
}

void NET_GetLocalAddress (void)
{
	char	buff[512];
	struct sockaddr_in	address;
	int		namelen;

	gethostname(buff, 512);
	buff[512-1] = 0;

	NET_StringToAdr (buff, &net_local_adr);

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error ("NET_Init: getsockname:", strerror(errno));
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
	WORD	wVersionRequested; 
	int		r;

#ifdef _DEBUG
	ZeroFreq();
#endif

	BuildTree(HuffFreq);

	wVersionRequested = MAKEWORD(1, 1); 

	r = WSAStartup (MAKEWORD(1, 1), &winsockdata);

	if (r)
		Sys_Error ("Winsock initialization failed.");

	//
	// open the single socket to be used for all communications
	//
	net_socket = UDP_OpenSocket (port);

	//
	// init the message buffer
	//
	net_message.maxsize = sizeof(net_message_buffer);
	net_message._data = net_message_buffer;

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
	WSACleanup ();

#ifdef _DEBUG
	PrintFreqs();
#endif
}
