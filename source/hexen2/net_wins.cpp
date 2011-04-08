// net_wins.c

#include "quakedef.h"
#include "winquake.h"

extern QCvar* hostname;

#define MAXHOSTNAMELEN		256

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
//static qboolean ifbcastinit = false;
static struct qsockaddr broadcastaddr;

static unsigned long myAddr;

#include "net_wins.h"

int winsock_initialized = 0;
WSADATA		winsockdata;

//=============================================================================

static double	blocktime;

BOOL PASCAL FAR BlockingHook(void)  
{ 
    MSG		msg;
    BOOL	ret;
 
	if ((Sys_FloatTime() - blocktime) > 2.0)
	{
		WSACancelBlockingCall();
		return FALSE;
	}

    /* get the next message, if any */ 
    ret = (BOOL) PeekMessage(&msg, NULL, 0, 0, PM_REMOVE); 
 
    /* if we got one, process it */ 
    if (ret) { 
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    } 
 
    /* TRUE if we got a message */ 
    return ret; 
} 

int WINS_Init (void)
{
	int		i;
	struct hostent *local = NULL;
	char	buff[MAXHOSTNAMELEN];
	struct qsockaddr addr;
	char	*p;
	int		r;
	WORD	wVersionRequested;

	if (COM_CheckParm ("-noudp"))
		return -1;

	if (winsock_initialized == 0)
	{
		wVersionRequested = MAKEWORD(1, 1); 

		r = WSAStartup (MAKEWORD(1, 1), &winsockdata);

		if (r)
		{
			Con_SafePrintf ("Winsock initialization failed.\n");
			return -1;
		}
	}
	winsock_initialized++;

	// determine my name & address
	if (gethostname(buff, MAXHOSTNAMELEN) == 0)
	{
		blocktime = Sys_FloatTime();
		WSASetBlockingHook(BlockingHook);
		local = gethostbyname(buff);
		WSAUnhookBlockingHook();
		if (local == NULL)
		{
			Con_DPrintf ("Winsock TCP/IP Initialization timed out.\n");
			if (--winsock_initialized == 0)
				WSACleanup ();
			return -1;
		}
	}

	if (local)
	{
		myAddr = *(int *)local->h_addr_list[0];

		// if the quake hostname isn't set, set it to the machine name
		if (QStr::Cmp(hostname->string, "UNNAMED") == 0)
		{
			// see if it's a text IP address (well, close enough)
			for (p = buff; *p; p++)
				if ((*p < '0' || *p > '9') && *p != '.')
					break;

			// if it is a real name, strip off the domain; we only want the host
			if (*p)
			{
				for (i = 0; i < 15; i++)
					if (buff[i] == '.')
						break;
				buff[i] = 0;
			}
			Cvar_Set ("hostname", buff);
		}
	}

	if ((net_controlsocket = WINS_OpenSocket (0)) == -1)
	{
		Con_Printf("WINS_Init: Unable to open control socket\n");
		if (--winsock_initialized == 0)
			WSACleanup ();
		return -1;
	}

	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons((unsigned short)net_hostport);

	WINS_GetSocketAddr (net_controlsocket, &addr);
	QStr::Cpy(my_tcpip_address,  WINS_AddrToString (&addr));
	p = QStr::RChr(my_tcpip_address, ':');
	if (p)
		*p = 0;

	Con_Printf("Winsock TCP/IP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void WINS_Shutdown (void)
{
	WINS_Listen (false);
	WINS_CloseSocket (net_controlsocket);
	if (--winsock_initialized == 0)
	{
		WSACleanup ();
	}
}

//=============================================================================

void WINS_Listen (qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = WINS_OpenSocket (net_hostport)) == -1)
			Sys_Error ("WINS_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	WINS_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int WINS_OpenSocket (int port)
{
	int newsocket = SOCK_Open(NULL, port);
	if (newsocket == 0)
		return -1;
	return newsocket;
}

//=============================================================================

int WINS_CloseSocket (int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	SOCK_Close(socket);
	return 0;
}


//=============================================================================

int WINS_Connect (int socket, struct qsockaddr *addr)
{
	return 0;
}

//=============================================================================

int WINS_CheckNewConnections (void)
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

int WINS_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int addrlen = sizeof (struct qsockaddr);
	int ret;

	ret = recvfrom (socket, (char*)buf, len, 0, (struct sockaddr *)addr, &addrlen);
	if (ret == -1)
	{
		int err = WSAGetLastError();

		if (err == WSAEWOULDBLOCK || err == WSAECONNREFUSED)
			return 0;

	}
	return ret;
}

//=============================================================================

int WINS_MakeSocketBroadcastCapable (int socket)
{
	net_broadcastsocket = socket;
	return 0;
}

//=============================================================================

int WINS_Broadcast (int socket, byte *buf, int len)
{
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		ret = WINS_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}

	return WINS_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int WINS_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret;

	ret = sendto (socket, (char*)buf, len, 0, (struct sockaddr *)addr, sizeof(struct qsockaddr));
	if (ret == -1)
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return 0;

	return ret;
}

//=============================================================================

char *WINS_AddrToString (struct qsockaddr *addr)
{
	static char buffer[22];
	int haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff, ntohs(((struct sockaddr_in *)addr)->sin_port));
	return buffer;
}

//=============================================================================

int WINS_StringToAddr (char *string, struct qsockaddr *addr)
{
	int ha1, ha2, ha3, ha4, hp;
	int ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)hp);
	return 0;
}

//=============================================================================

int WINS_GetSocketAddr (int socket, struct qsockaddr *addr)
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

int WINS_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
	struct hostent *hostentry;

	hostentry = gethostbyaddr ((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		QStr::NCpy(name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}

	QStr::Cpy(name, WINS_AddrToString (addr));
	return 0;
}

//=============================================================================

int WINS_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	if (!SOCK_StringToSockaddr(name, (struct sockaddr_in*)addr))
		return -1;

	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)net_hostport);	

	return 0;
}

//=============================================================================

int WINS_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
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

int WINS_GetSocketPort (struct qsockaddr *addr)
{
	return ntohs(((struct sockaddr_in *)addr)->sin_port);
}


int WINS_SetSocketPort (struct qsockaddr *addr, int port)
{
	((struct sockaddr_in *)addr)->sin_port = htons((unsigned short)port);
	return 0;
}

//=============================================================================
