// net_wins.c

#include "../qcommon/qcommon.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#include <sys/filio.h>

#ifdef NeXT
#include <libc.h>
#endif

extern int			ip_sockets[2];

// sleeps msec or until net socket is ready
void NET_Sleep(int msec)
{
	extern QCvar *dedicated;

	if (dedicated && !dedicated->value)
	{
		return; // we're not a server, just run full speed
	}

	SOCK_Sleep(ip_sockets[NS_SERVER], msec);
}
