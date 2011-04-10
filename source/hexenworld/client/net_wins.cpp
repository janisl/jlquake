// net_wins.c

#include "quakedef.h"
#include "winquake.h"

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
