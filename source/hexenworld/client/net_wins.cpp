// net_wins.c

#include "quakedef.h"
#include "winquake.h"

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
