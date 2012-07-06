/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"
#include "server.h"

/*
=================
SV_Netchan_TransmitNextFragment
=================
*/
void SV_Netchan_TransmitNextFragment(netchan_t* chan)
{
	Netchan_TransmitNextFragment(chan);
}


/*
===============
SV_Netchan_Transmit
================
*/

//extern byte chksum[65536];
void SV_Netchan_Transmit(client_t* client, QMsg* msg)		//int length, const byte *data ) {
{
	msg->WriteByte(q3svc_EOF);
	Netchan_Transmit(&client->netchan, msg->cursize, msg->_data);
}

/*
=================
Netchan_SV_Process
=================
*/
qboolean SV_Netchan_Process(client_t* client, QMsg* msg)
{
	int ret;
	ret = Netchan_Process(&client->netchan, msg);
	if (!ret)
	{
		return qfalse;
	}
	return qtrue;
}
