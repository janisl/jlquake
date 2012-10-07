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

bool clhw_siege;
int clhw_keyholder = -1;
int clhw_doc = -1;
float clhw_timelimit;
float clhw_server_time_offset;
byte clhw_fraglimit;
unsigned int clhw_defLosses;		// Defenders losses in Siege
unsigned int clhw_attLosses;		// Attackers Losses in Siege

//	Server information pertaining to this client only
void CLH2_ParseClientdata(QMsg& message)
{
	int bits = message.ReadShort();

	if (bits & Q1SU_VIEWHEIGHT)
	{
		cl.qh_viewheight = message.ReadChar();
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		cl.qh_idealpitch = message.ReadChar();
	}
	else
	{
	}

	if (bits & H2SU_IDEALROLL)
	{
		cl.h2_idealroll = message.ReadChar();
	}

	VectorCopy(cl.qh_mvelocity[0], cl.qh_mvelocity[1]);
	for (int i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			cl.qh_punchangles[i] = message.ReadChar();
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			cl.qh_mvelocity[0][i] = message.ReadChar() * 16;
		}
	}

	cl.qh_onground = (bits & Q1SU_ONGROUND) != 0;

	if (bits & Q1SU_WEAPONFRAME)
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = message.ReadByte();
	}

	if (bits & Q1SU_ARMOR)
	{
		cl.qh_stats[Q1STAT_ARMOR] = message.ReadByte();
	}

	if (bits & Q1SU_WEAPON)
	{
		cl.qh_stats[Q1STAT_WEAPON] = message.ReadShort();
	}
}
