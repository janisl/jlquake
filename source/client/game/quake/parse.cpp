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

//	Server information pertaining to this client only
void CLQ1_ParseClientdata(QMsg& message)
{
	int bits = message.ReadShort();

	if (bits & Q1SU_VIEWHEIGHT)
	{
		cl.qh_viewheight = message.ReadChar();
	}
	else
	{
		cl.qh_viewheight = Q1DEFAULT_VIEWHEIGHT;
	}

	if (bits & Q1SU_IDEALPITCH)
	{
		cl.qh_idealpitch = message.ReadChar();
	}
	else
	{
		cl.qh_idealpitch = 0;
	}

	VectorCopy(cl.qh_mvelocity[0], cl.qh_mvelocity[1]);
	for (int i = 0; i < 3; i++)
	{
		if (bits & (Q1SU_PUNCH1 << i))
		{
			cl.qh_punchangles[i] = message.ReadChar();
		}
		else
		{
			cl.qh_punchangles[i] = 0;
		}
		if (bits & (Q1SU_VELOCITY1 << i))
		{
			cl.qh_mvelocity[0][i] = message.ReadChar() * 16;
		}
		else
		{
			cl.qh_mvelocity[0][i] = 0;
		}
	}

	// [always sent]	if (bits & Q1SU_ITEMS)
	int i = message.ReadLong();

	if (cl.q1_items != i)
	{
		// set flash times
		for (int j = 0; j < 32; j++)
		{
			if ((i & (1 << j)) && !(cl.q1_items & (1 << j)))
			{
				cl.q1_item_gettime[j] = cl.qh_serverTimeFloat;
			}
		}
		cl.q1_items = i;
	}

	cl.qh_onground = (bits & Q1SU_ONGROUND) != 0;

	if (bits & Q1SU_WEAPONFRAME)
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = message.ReadByte();
	}
	else
	{
		cl.qh_stats[Q1STAT_WEAPONFRAME] = 0;
	}

	if (bits & Q1SU_ARMOR)
	{
		i = message.ReadByte();
	}
	else
	{
		i = 0;
	}
	if (cl.qh_stats[Q1STAT_ARMOR] != i)
	{
		cl.qh_stats[Q1STAT_ARMOR] = i;
	}

	if (bits & Q1SU_WEAPON)
	{
		i = message.ReadByte();
	}
	else
	{
		i = 0;
	}
	if (cl.qh_stats[Q1STAT_WEAPON] != i)
	{
		cl.qh_stats[Q1STAT_WEAPON] = i;
	}

	i = message.ReadShort();
	if (cl.qh_stats[Q1STAT_HEALTH] != i)
	{
		cl.qh_stats[Q1STAT_HEALTH] = i;
	}

	i = message.ReadByte();
	if (cl.qh_stats[Q1STAT_AMMO] != i)
	{
		cl.qh_stats[Q1STAT_AMMO] = i;
	}

	for (i = 0; i < 4; i++)
	{
		int j = message.ReadByte();
		if (cl.qh_stats[Q1STAT_SHELLS + i] != j)
		{
			cl.qh_stats[Q1STAT_SHELLS + i] = j;
		}
	}

	i = message.ReadByte();

	if (q1_standard_quake)
	{
		if (cl.qh_stats[Q1STAT_ACTIVEWEAPON] != i)
		{
			cl.qh_stats[Q1STAT_ACTIVEWEAPON] = i;
		}
	}
	else
	{
		if (cl.qh_stats[Q1STAT_ACTIVEWEAPON] != (1 << i))
		{
			cl.qh_stats[Q1STAT_ACTIVEWEAPON] = (1 << i);
		}
	}
}
