// Main windowed and fullscreen graphics interface module. This module
// is used for both the software and OpenGL rendering versions of the
// Quake refresh engine.

#include <assert.h>
#include <dlfcn.h> // ELF dl loader
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "../client/client.h"
#include "../ref_gl/gl_local.h"

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
	vid_ref = Cvar_Get( "vid_ref", "soft", CVAR_ARCHIVE );
	if ( vid_ref->modified )
	{
		S_StopAllSounds();

		/*
		** refresh has changed
		*/
		vid_ref->modified = false;
		//FIXME
		if (r_fullscreen)
			r_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		if (!VID_LoadRefresh())
		{
			Com_Error(ERR_FATAL, "Couldn't init refresh!");
		}
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

	/* Start the graphics mode and load refresh DLL */
	VID_CheckChanges();
}
