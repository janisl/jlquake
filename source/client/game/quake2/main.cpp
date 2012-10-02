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

#include "../../client.h"
#include "local.h"

Cvar* q2_hand;
Cvar* clq2_footsteps;
Cvar* clq2_name;
Cvar* clq2_skin;
Cvar* clq2_vwep;
Cvar* clq2_predict;

q2centity_t clq2_entities[MAX_EDICTS_Q2];

void CLQ2_PingServers_f()
{
	NET_Config(true);		// allow remote

	// send a broadcast packet
	common->Printf("pinging broadcast...\n");

	Cvar* noudp = Cvar_Get("noudp", "0", CVAR_INIT);
	if (!noudp->value)
	{
		netadr_t adr;
		adr.type = NA_BROADCAST;
		adr.port = BigShort(Q2PORT_SERVER);
		NET_OutOfBandPrint(NS_CLIENT, adr, "info %i", Q2PROTOCOL_VERSION);
	}

	// send a packet to each address book entry
	for (int i = 0; i < 16; i++)
	{
		char name[32];
		String::Sprintf(name, sizeof(name), "adr%i", i);
		const char* adrstring = Cvar_VariableString(name);
		if (!adrstring || !adrstring[0])
		{
			continue;
		}

		common->Printf("pinging %s...\n", adrstring);
		netadr_t adr;
		if (!SOCK_StringToAdr(adrstring, &adr, Q2PORT_SERVER))
		{
			common->Printf("Bad address: %s\n", adrstring);
			continue;
		}
		NET_OutOfBandPrint(NS_CLIENT, adr, "info %i", Q2PROTOCOL_VERSION);
	}
}
