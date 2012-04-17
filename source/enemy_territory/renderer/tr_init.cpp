/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_init.c -- functions that are not called every frame

#include "../../wolfclient/client.h"
#include "../../client/renderer/local.h"
#include "tr_public.h"

void R_PurgeCache( void ) {
	R_PurgeShaders();
	R_PurgeBackupImages( 9999999 );
	R_PurgeModels( 9999999 );
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown( qboolean destroyWindow ) {

	common->Printf("R_Shutdown( %i )\n", destroyWindow );

	Cmd_RemoveCommand( "modellist" );
	Cmd_RemoveCommand( "screenshotJPEG" );
	Cmd_RemoveCommand( "screenshot" );
	Cmd_RemoveCommand( "imagelist" );
	Cmd_RemoveCommand( "shaderlist" );
	Cmd_RemoveCommand( "skinlist" );
	Cmd_RemoveCommand( "gfxinfo" );
	Cmd_RemoveCommand( "modelist" );
	Cmd_RemoveCommand( "shaderstate" );

	// Ridah
	Cmd_RemoveCommand( "cropimages" );
	// done.

	R_ShutdownCommandBuffers();

	// Ridah, keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeCache();

	if (r_cache->integer && tr.registered && !destroyWindow)
	{
		R_BackupModels();
		R_BackupShaders();
		R_BackupImages();
	}
	if (tr.registered)
	{
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();

		R_FreeModels();

		R_FreeShaders();

		R_DeleteTextures();

		R_FreeBackEndData();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		// shutdown QGL subsystem
		QGL_Shutdown();
	}

	tr.registered = false;
}


/*
=============
R_EndRegistration

Touch all images to make sure they are resident
=============
*/
void R_EndRegistration( void ) {
	R_SyncRenderThread();
//		RB_ShowImages();
}
