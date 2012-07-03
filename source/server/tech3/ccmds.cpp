//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../server.h"
#include "local.h"
//#include "../quake3/local.h"
//#include "../wolfsp/local.h"
//#include "../wolfmp/local.h"
//#include "../et/local.h"

//	Also called by SVT3_DropClient, SV_DirectConnect, and SV_SpawnServer
void SVT3_Heartbeat_f()
{
	svs.q3_nextHeartbeatTime = -9999999;
}

void SVET_TempBanNetAddress(netadr_t address, int length)
{
	int oldesttime = 0;
	int oldest = -1;
	for (int i = 0; i < MAX_TEMPBAN_ADDRESSES; i++)
	{
		if (!svs.et_tempBanAddresses[i].endtime || svs.et_tempBanAddresses[i].endtime < svs.q3_time)
		{
			// found a free slot
			svs.et_tempBanAddresses[i].adr = address;
			svs.et_tempBanAddresses[i].endtime = svs.q3_time + (length * 1000);
			return;
		}
		else
		{
			if (oldest == -1 || oldesttime > svs.et_tempBanAddresses[i].endtime)
			{
				oldesttime = svs.et_tempBanAddresses[i].endtime;
				oldest = i;
			}
		}
	}

	svs.et_tempBanAddresses[oldest].adr = address;
	svs.et_tempBanAddresses[oldest].endtime = svs.q3_time + length;
}
