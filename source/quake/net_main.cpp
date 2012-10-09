/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_main.c

#include "quakedef.h"
#include "../server/server.h"
#include "../server/quake_hexen/local.h"

QMsg net_message;
byte net_message_buf[MAX_MSGLEN_Q1];

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
	net_message.InitOOB(net_message_buf, MAX_MSGLEN_Q1);

#ifndef DEDICATED
	Cmd_AddCommand("slist", NET_Slist_f);
#endif
	Cmd_AddCommand("maxplayers", MaxPlayers_f);
}
