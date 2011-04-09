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

extern int			ip_sockets[2];

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

