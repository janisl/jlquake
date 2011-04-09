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
		Sys_Error("NET_GetPacket failed");
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
	char string[120];

	HuffEncode((unsigned char *)data,huffbuff,length,&outlen);

#ifdef _DEBUG
	sprintf(string,"in: %d  out: %d  ratio: %f\n",HuffIn, HuffOut, 1-(float)HuffOut/(float)HuffIn);
	OutputDebugString(string);

	CalcFreq((unsigned char *)data, length);
#endif

	SOCL_Send(net_socket, huffbuff, outlen, &to);
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
	struct sockaddr_in	address;
	int		namelen;

	SOCK_GetLocalAddress();

	struct sockaddr_in sadr;
	
	Com_Memset(&sadr, 0, sizeof(sadr));

	sadr.sin_family = AF_INET;
	sadr.sin_port = 0;

	*(int*)&sadr.sin_addr = *(int*)localIP[0];

	SockadrToNetadr (&sadr, &net_local_adr);

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == -1)
		Sys_Error ("NET_Init: getsockname:", strerror(errno));
	net_local_adr.port = address.sin_port;
}

/*
====================
NET_Init
====================
*/
void NET_Init (int port)
{
#ifdef _DEBUG
	ZeroFreq();
#endif

	BuildTree(HuffFreq);

	if (!SOCK_Init())
		Sys_Error ("Sockets initialization failed.");

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
}

/*
====================
NET_Shutdown
====================
*/
void	NET_Shutdown (void)
{
	SOCK_Close(net_socket);
	SOCK_Shutdown();

#ifdef _DEBUG
	PrintFreqs();
#endif
}
