// net_wins.c

#include "quakedef.h"
#include "winquake.h"

void NET_GetLocalAddress (void)
{
	SOCK_GetLocalAddress();

	struct sockaddr_in sadr;
	
	Com_Memset(&sadr, 0, sizeof(sadr));

	sadr.sin_family = AF_INET;
	sadr.sin_port = 0;

	*(int*)&sadr.sin_addr = *(int*)localIP[0];

	SockadrToNetadr (&sadr, &net_local_adr);

	netadr_t address;
	if (!SOCK_GetAddr(net_socket, &address))
		Sys_Error("NET_Init: getsockname failed");
	net_local_adr.port = address.port;
}
