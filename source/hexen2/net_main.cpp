// net_main.c

#include "quakedef.h"

QMsg net_message;
byte net_message_buf[MAX_MSGLEN_H2];

//=============================================================================

/*
====================
NET_Init
====================
*/

void NET_Init(void)
{
	net_numsockets = svs.qh_maxclientslimit;
	NET_CommonInit();

	// allocate space for network message buffer
	net_message.InitOOB(net_message_buf, MAX_MSGLEN_H2);

	Cmd_AddCommand("slist", NET_Slist_f);
	Cmd_AddCommand("maxplayers", MaxPlayers_f);
}
