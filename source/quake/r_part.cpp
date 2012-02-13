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

#include "quakedef.h"

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord();
	for (i=0 ; i<3 ; i++)
		dir[i] = net_message.ReadChar () * (1.0/16);
	count = net_message.ReadByte ();
	color = net_message.ReadByte ();

	if (count == 255)
	{
		// rocket explosion
		CLQ1_ParticleExplosion(org);
	}
	else
	{
		CLQ1_RunParticleEffect (org, dir, color, count);
	}
}
