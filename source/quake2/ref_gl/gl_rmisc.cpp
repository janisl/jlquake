/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// r_misc.c

#include "gl_local.h"
#include "../client/client.h"

refexport_t	re;

qboolean	reflib_active = 0;
static bool vid_restart_requested;

/*
============
VID_Restart_f

Console command to re-start the video mode and refresh.
============
*/
void VID_Restart_f (void)
{
	vid_restart_requested = true;
}

/*
** VID_NewWindow
*/
void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;
}

void VID_FreeReflib (void)
{
	Com_Memset(&re, 0, sizeof(re));
	reflib_active  = false;
}

/*
==============
VID_LoadRefresh
==============
*/
static void VID_LoadRefresh()
{
	if ( reflib_active )
	{
		R_Shutdown(true);
		VID_FreeReflib ();
	}

	re = GetRefAPI();

	CL_InitRenderStuff();

	Com_Printf( "------------------------------------\n");
	reflib_active = true;
}

/*
============
VID_Shutdown
============
*/
void VID_Shutdown (void)
{
	if ( reflib_active )
	{
		R_Shutdown(true);
		VID_FreeReflib ();
	}
}

/*
============
VID_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to 
update the rendering DLL and/or video mode to match.
============
*/
void VID_CheckChanges (void)
{
	if (vid_restart_requested)
	{
		S_StopAllSounds();
		/*
		** refresh has changed
		*/
		vid_restart_requested = false;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		VID_LoadRefresh();
		cls.disable_screen = false;
	}
}

/*
============
VID_Init
============
*/
void VID_Init (void)
{
	/* Add some console commands that we want to handle */
	Cmd_AddCommand ("vid_restart", VID_Restart_f);

	VID_LoadRefresh();
}
