/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include "../../client/game/quake_hexen2/demo.h"

/*
====================
CL_ReRecord_f

record <demoname>
====================
*/
void CL_ReRecord_f(void)
{
	int c;
	char name[MAX_OSPATH];

	c = Cmd_Argc();
	if (c != 2)
	{
		common->Printf("rerecord <demoname>\n");
		return;
	}

	if (!*cls.servername)
	{
		common->Printf("No server to reconnect to...\n");
		return;
	}

	if (clc.demorecording)
	{
		CLQH_Stop_f();
	}

	String::Cpy(name, Cmd_Argv(1));

//
// open the demo file
//
	String::DefaultExtension(name, sizeof(name), ".qwd");

	clc.demofile = FS_FOpenFileWrite(name);
	if (!clc.demofile)
	{
		common->Printf("ERROR: couldn't open.\n");
		return;
	}

	common->Printf("recording to %s.\n", name);
	clc.demorecording = true;

	CL_Disconnect(true);
	CL_BeginServerConnect();
}
